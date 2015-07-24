/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    init.c++
    Start the operating system.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/display.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fstab.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <fsmarshall.h>

#include <mount/blockdevice.h>
#include <mount/devices.h>
#include <mount/filesystem.h>
#include <mount/harddisk.h>
#include <mount/partition.h>
#include <mount/uuid.h>

static char* read_single_line(FILE* fp)
{
	char* ret = NULL;
	size_t ret_size = 0;
	ssize_t ret_length = getline(&ret, &ret_size, fp);
	if ( ret_length < 0 )
	{
		free(ret);
		return NULL;
	}
	if ( ret_length && ret[ret_length-1] == '\n' )
		ret[--ret_length] = '\0';
	return ret;
}

static char* join_paths(const char* a, const char* b)
{
	size_t a_len = strlen(a);
	bool has_slash = (a_len && a[a_len-1] == '/') || b[0] == '/';
	char* result;
	if ( (has_slash && asprintf(&result, "%s%s", a, b) < 0) ||
	     (!has_slash && asprintf(&result, "%s/%s", a, b) < 0) )
		return NULL;
	return result;
}

__attribute__((noreturn))
__attribute__((format(printf, 1, 2)))
static void fatal(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "%s: fatal: ", program_invocation_name);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(ap);
	_exit(2);
}

__attribute__((format(printf, 1, 2)))
static void warning(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "%s: warning: ", program_invocation_name);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(ap);
}

__attribute__((format(printf, 1, 2)))
static void note(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "%s: ", program_invocation_name);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(ap);
}

struct harddisk** hds = NULL;
size_t hds_used = 0;
size_t hds_length = 0;

void prepare_filesystem(const char* path, struct blockdevice* bdev)
{
	enum filesystem_error fserr = blockdevice_inspect_filesystem(&bdev->fs, bdev);
	if ( fserr == FILESYSTEM_ERROR_ABSENT ||
	     fserr == FILESYSTEM_ERROR_UNRECOGNIZED )
		return;
	if ( fserr != FILESYSTEM_ERROR_NONE )
		return warning("probing: %s: %s", path, filesystem_error_string(fserr));
}

bool prepare_block_device(void* ctx, const char* path)
{
	(void) ctx;
	struct harddisk* hd = harddisk_openat(AT_FDCWD, path, O_RDONLY);
	if ( !hd )
	{
		int true_errno = errno;
		struct stat st;
		if ( lstat(path, &st) == 0 && !S_ISBLK(st.st_mode) )
			return true;
		errno = true_errno;
		fatal("%s: %m", path);
	}
	if ( !harddisk_inspect_blockdevice(hd) )
	{
		if ( errno == ENOTBLK )
			return true;
		if ( errno == EINVAL )
			return warning("%s: %m", path), true;
		fatal("%s: %m", path);
	}
	if ( hds_used == hds_length )
	{
		// TODO: Potential overflow.
		size_t new_length = hds_length ? 2 * hds_length : 16;
		struct harddisk** new_hds = (struct harddisk**)
			reallocarray(hds, new_length, sizeof(struct harddisk*));
		if ( !new_hds )
			fatal("realloc: %m");
		hds = new_hds;
		hds_length = new_length;
	}
	hds[hds_used++] = hd;
	struct blockdevice* bdev = &hd->bdev;
	enum partition_error parterr = blockdevice_get_partition_table(&bdev->pt, bdev);
	if ( parterr == PARTITION_ERROR_ABSENT ||
	     parterr == PARTITION_ERROR_UNRECOGNIZED )
	{
		prepare_filesystem(path, bdev);
		return true;
	}
	else if ( parterr == PARTITION_ERROR_ERRNO )
	{
		if ( errno == EIO || errno == EINVAL )
			warning("%s: %s", path, partition_error_string(parterr));
		else
			fatal("%s: %s", path, partition_error_string(parterr));
		return true;
	}
	else if ( parterr != PARTITION_ERROR_NONE )
	{
		warning("%s: %s", path, partition_error_string(parterr));
		return true;
	}
	for ( size_t i = 0; i < bdev->pt->partitions_count; i++ )
	{
		struct partition* p = bdev->pt->partitions[i];
		assert(p->path);
		struct stat st;
		if ( stat(p->path, &st) == 0 )
		{
			// TODO: Check the existing partition has the right offset and
			//       length, but definitely do not recreate it if it already
			//       exists properly.
		}
		else if ( errno == ENOENT )
		{
			int mountfd = open(p->path, O_RDONLY | O_CREAT | O_EXCL);
			if ( mountfd < 0 )
				fatal("%s:˙%m", p->path);
			int partfd = mkpartition(hd->fd, p->start, p->length);
			if ( partfd < 0 )
				fatal("mkpartition: %s:˙%m", p->path);
			if ( fsm_fsbind(partfd, mountfd, 0) < 0 )
				fatal("fsbind: %s:˙%m", p->path);
			close(partfd);
			close(mountfd);
		}
		else
		{
			fatal("stat: %s: %m", p->path);
		}
		prepare_filesystem(p->path, &p->bdev);
	}
	return true;
}

void prepare_block_devices()
{
	static bool done = false;
	if ( done )
		return;
	done = true;

	if ( !devices_iterate_path(prepare_block_device, NULL) )
		fatal("iterating devices: %m");
}

struct device_match
{
	const char* path;
	struct blockdevice* bdev;
};

void search_by_uuid(const char* uuid_string,
                    void (*cb)(void*, struct device_match*),
                    void* ctx)
{
	unsigned char uuid[16];
	uuid_from_string(uuid, uuid_string);
	for ( size_t i = 0; i < hds_used; i++ )
	{
		struct blockdevice* bdev = &hds[i]->bdev;
		if ( bdev->fs )
		{
			struct filesystem* fs = bdev->fs;
			if ( !(fs->flags & FILESYSTEM_FLAG_UUID) )
				continue;
			if ( memcmp(uuid, fs->uuid, 16) != 0 )
				continue;
			struct device_match match;
			match.path = hds[i]->path;
			match.bdev = bdev;
			cb(ctx, &match);
		}
		else if ( bdev->pt )
		{
			for ( size_t j = 0; j < bdev->pt->partitions_count; j++ )
			{
				struct partition* p = bdev->pt->partitions[j];
				if ( !p->bdev.fs )
					continue;
				struct filesystem* fs = p->bdev.fs;
				if ( !(fs->flags & FILESYSTEM_FLAG_UUID) )
					continue;
				if ( memcmp(uuid, fs->uuid, 16) != 0 )
					continue;
				struct device_match match;
				match.path = p->path;
				match.bdev = &p->bdev;
				cb(ctx, &match);
			}
		}
	}
}

void ensure_single_device_match(void* ctx, struct device_match* match)
{
	struct device_match* result = (struct device_match*) ctx;
	if ( result->path )
	{
		if ( result->bdev )
			note("duplicate match: %s", result->path);
		result->bdev = NULL;
		note("duplicate match: %s", match->path);
		return;
	}
	*result = *match;
}

void set_hostname()
{
	FILE* fp = fopen("/etc/hostname", "r");
	if ( !fp && errno == ENOENT )
		return;
	if ( !fp )
		return warning("unable to set hostname: /etc/hostname: %m");
	char* hostname = read_single_line(fp);
	if ( !hostname )
		return warning("unable to set hostname: /etc/hostname: %m");
	fclose(fp);
	int ret = sethostname(hostname, strlen(hostname) + 1);
	free(hostname);
	if ( ret < 0 )
		return warning("unable to set hostname: `%s': %m", hostname);
}

void set_kblayout()
{
	FILE* fp = fopen("/etc/kblayout", "r");
	if ( !fp && errno == ENOENT )
		return;
	if ( !fp )
		return warning("unable to set keyboard layout: /etc/kblayout: %m");
	char* kblayout = read_single_line(fp);
	if ( !kblayout )
		return warning("unable to set keyboard layout: /etc/kblayout: %m");
	fclose(fp);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return warning("unable to set keyboard layout: fork: %m");
	if ( !child_pid )
	{
		execlp("chkblayout", "chkblayout", "--", kblayout, (char*) NULL);
		warning("setting keyboard layout: chkblayout: %m");
		_exit(127);
	}
	int status;
	waitpid(child_pid, &status, 0);
	free(kblayout);
}

void set_videomode()
{
	FILE* fp = fopen("/etc/videomode", "r");
	if ( !fp && errno == ENOENT )
		return;
	if ( !fp )
		return warning("unable to set video mode: /etc/videomode: %m");
	char* videomode = read_single_line(fp);
	if ( !videomode )
		return warning("unable to set video mode: /etc/videomode: %m");
	fclose(fp);
	unsigned int xres = 0;
	unsigned int yres = 0;
	unsigned int bpp = 0;
	if ( sscanf(videomode, "%ux%ux%u", &xres, &yres, &bpp) != 3 )
	{
		warning("/etc/videomode: Invalid video mode `%s'", videomode);
		free(videomode);
		return;
	}
	free(videomode);
	struct dispmsg_set_crtc_mode set_mode;
	memset(&set_mode, 0, sizeof(set_mode));
	set_mode.msgid = DISPMSG_SET_CRTC_MODE;
	set_mode.device = 0;
	set_mode.connector = 0;
	set_mode.mode.driver_index = 0;
	set_mode.mode.magic = 0;
	set_mode.mode.control = DISPMSG_CONTROL_VALID;
	set_mode.mode.fb_format = bpp;
	set_mode.mode.view_xres = xres;
	set_mode.mode.view_yres = yres;
	set_mode.mode.fb_location = 0;
	set_mode.mode.pitch = xres * (bpp / 8);
	set_mode.mode.surf_off_x = 0;
	set_mode.mode.surf_off_y = 0;
	set_mode.mode.start_x = 0;
	set_mode.mode.start_y = 0;
	set_mode.mode.end_x = 0;
	set_mode.mode.end_y = 0;
	set_mode.mode.desktop_height = yres;
	if ( dispmsg_issue(&set_mode, sizeof(set_mode)) < 0 )
		warning("/etc/videomode: Failed to set video mode `%ux%ux%u': %m",
		        xres, yres, bpp);
}

void init_early()
{
	static bool done = false;
	if ( done )
		return;
	done = true;

	// Make sure that we have a /tmp directory.
	umask(0000);
	mkdir("/tmp", 01777);

	// Set the default file creation mask.
	umask(0022);

	// Set up the PATH variable.
	if ( setenv("PATH", "/bin:/sbin", 1) < 0 )
		fatal("setenv: %m");

	// Set the terminal type.
	if ( setenv("TERM", "sortix", 1) < 0 )
		fatal("setenv: %m");
}

int init(const char* target)
{
	if ( getenv("INIT_PID") )
		fatal("System is already managed by an init process");
	init_early();
	set_hostname();
	set_kblayout();
	set_videomode();
	prepare_block_devices();
	sigset_t oldset, sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	while ( true )
	{
		struct termios tio;
		if ( tcgetattr(0, &tio) )
			fatal("tcgetattr: %m");
		pid_t child_pid = fork();
		if ( child_pid < 0 )
			fatal("fork: %m");
		if ( !child_pid )
		{
			uid_t uid = getuid();
			pid_t pid = getpid();
			pid_t ppid = getppid();
			if ( setpgid(0, 0) < 0 )
				fatal("setpgid: %m");
			sigprocmask(SIG_BLOCK, &sigttou, &oldset);
			if ( tcsetpgrp(0, pid) < 0 )
				fatal("tcsetpgrp: %m");
			sigprocmask(SIG_SETMASK, &oldset, NULL);
			struct passwd* pwd = getpwuid(uid);
			if ( !pwd )
				fatal("looking up user by uid %" PRIuUID ": %m", uid);
			const char* home = pwd->pw_dir[0] ? pwd->pw_dir : "/";
			const char* shell = pwd->pw_shell[0] ? pwd->pw_shell : "sh";
			char ppid_str[sizeof(pid_t) * 3];
			snprintf(ppid_str, sizeof(ppid_str), "%" PRIiPID, ppid);
			if ( setenv("INIT_PID", ppid_str, 1) < 0 ||
			     setenv("LOGNAME", pwd->pw_name, 1) < 0 ||
			     setenv("USER", pwd->pw_name, 1) < 0 ||
			     setenv("HOME", home, 1) < 0 ||
			     setenv("SHELL", shell, 1) < 0 )
				fatal("setenv: %m");
			if ( chdir(home) < 0 )
				warning("chdir: %s: %m", home);
			const char* program = "login";
			bool activate_terminal = false;
			if ( !strcmp(target, "single-user") )
			{
				activate_terminal = true;
				program = shell;
			}
			if ( activate_terminal )
			{
				tio.c_cflag |= CREAD;
				if ( tcsetattr(0, TCSANOW, &tio) )
					fatal("tcgetattr: %m");
			}
			const char* argv[] = { program, NULL };
			execvp(program, (char* const*) argv);
			fatal("%s: %m", program);
		}
		int status;
		if ( waitpid(child_pid, &status, 0) < 0 )
			fatal("waitpid");
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		if ( tcsetattr(0, TCSAFLUSH, &tio) )
			fatal("tcgetattr: %m");
		if ( tcsetpgrp(0, getpgid(0)) < 0 )
			fatal("tcsetpgrp: %m");
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		const char* back = ": Trying to bring it back up again";
		if ( WIFEXITED(status) )
			return WEXITSTATUS(status);
		else if ( WIFSIGNALED(status) )
			note("session: %s%s", strsignal(WTERMSIG(status)), back);
		else
			note("session: Unexpected unusual termination%s", back);
	}
}

static pid_t chain_init_pid_by_path = -1;
static char* chain_mount_point_dev = NULL;
static void init_chain_by_path_unmount(void)
{
	if ( chain_init_pid_by_path != getpid() )
		return;
	if ( chain_mount_point_dev )
	{
		unmount(chain_mount_point_dev, 0);
		free(chain_mount_point_dev);
		chain_mount_point_dev = NULL;
	}
}

int init_chain_by_path(const char* path)
{
	struct stat rootst;
	struct stat pathst;
	if ( stat("/", &rootst) < 0 )
		fatal("stat: /: %m");
	if ( stat(path, &pathst) < 0 )
		fatal("stat: %s: %m", path);
	if ( rootst.st_dev == pathst.st_dev && rootst.st_ino == pathst.st_ino )
	{
		if ( getenv("INIT_PID") )
			fatal("System is already managed by an init process");
	}
	init_early();
	chain_init_pid_by_path = getpid();
	if ( atexit(init_chain_by_path_unmount) != 0 )
		fatal("atexit: %m");
	if ( !(chain_mount_point_dev = join_paths(path, "dev")) )
		fatal("asprintf: %m");
	if ( mkdir(chain_mount_point_dev, 755) < 0 && errno != EEXIST )
		fatal("mkdir: %s: %m", chain_mount_point_dev);
	int old_dev_fd = open("/dev", O_DIRECTORY | O_RDONLY);
	if ( old_dev_fd < 0 )
		fatal("%s: %m", "/dev");
	int new_dev_fd = open(chain_mount_point_dev, O_DIRECTORY | O_RDONLY);
	if ( new_dev_fd < 0 )
		fatal("%s: %m", chain_mount_point_dev);
	if ( fsm_fsbind(old_dev_fd, new_dev_fd, 0) < 0 )
		fatal("mount: `%s' onto `%s': %m", "/dev", chain_mount_point_dev);
	close(new_dev_fd);
	close(old_dev_fd);
	while ( true )
	{
		pid_t child_pid = fork();
		if ( child_pid < 0 )
			fatal("fork: %m");
		if ( !child_pid )
		{
			if ( chroot(path) < 0 )
				fatal("chroot: %s: %m", path);
			if ( chdir("/") < 0 )
				fatal("chdir: %s: %m", path);
			unsetenv("INIT_PID");
			const char* argv[] = { "init", NULL };
			execv("/sbin/init", (char* const*) argv);
			fatal("Failed to load chain init: %s: %m", argv[0]);
		}
		int status;
		if ( waitpid(child_pid, &status, 0) < 0 )
			fatal("waitpid");
		const char* back = ": Trying to bring it back up again";
		if ( WIFEXITED(status) )
			return WEXITSTATUS(status);
		else if ( WIFSIGNALED(status) )
			note("chain init: %s%s", strsignal(WTERMSIG(status)), back);
		else
			note("chain init: Unexpected unusual termination%s", back);
	}
}

static pid_t chain_init_pid_by_filesystem = -1;
static char* chain_mount_point = NULL;
static pid_t chain_mount_pid = -1;
static void init_chain_by_filesystem_unmount(void)
{
	if ( chain_init_pid_by_filesystem != getpid() )
		return;
	if ( chain_mount_point )
	{
		if ( unmount(chain_mount_point, 0) < 0 && errno != ENOMOUNT )
			warning("unmount: %s: %m", chain_mount_point);
		else if ( errno == ENOMOUNT && 0 <= chain_mount_pid )
			kill(chain_mount_pid, SIGQUIT);
	}
	if ( 0 <= chain_mount_pid )
	{
		int code;
		if ( waitpid(chain_mount_pid, &code, 0) < 0 )
			note("waitpid: %m");
		chain_mount_pid = -1;
	}
	if ( chain_mount_point )
	{
		if ( chain_mount_point && rmdir(chain_mount_point) < 0 )
			warning("rmdir: %s: %m", chain_mount_point);
		free(chain_mount_point);
	}
	chain_mount_point = NULL;
}

int init_chain_by_filesystem(struct filesystem* fs)
{
	// TODO: It would be ideal to get an exclusive lock so that no other
	//       processes have currently mounted that filesystem.
	struct blockdevice* bdev = fs->bdev;
	const char* bdev_path = bdev->p ? bdev->p->path : bdev->hd->path;
	assert(bdev_path);
	do if ( fs->flags & (FILESYSTEM_FLAG_FSCK_SHOULD | FILESYSTEM_FLAG_FSCK_MUST) )
	{
		assert(fs->fsck);
		if ( fs->flags & FILESYSTEM_FLAG_FSCK_MUST )
			note("%s: Repairing filesystem due to inconsistency...", bdev_path);
		else
			note("%s: Checking filesystem consistency...", bdev_path);
		pid_t child_pid = fork();
		if ( child_pid < 0 )
		{
			if ( fs->flags & FILESYSTEM_FLAG_FSCK_MUST )
				fatal("%s: Mandatory repair failed: fork: %m", bdev_path);
			warning("%s: Skipping filesystem check: fork: %m:", bdev_path);
			break;
		}
		if ( child_pid == 0 )
		{
			execlp(fs->fsck, fs->fsck, "-fp", "--", bdev_path, (const char*) NULL);
			note("%s: Failed to load filesystem checker: %s: %m", bdev_path, fs->fsck);
			_Exit(127);
		}
		int code;
		if ( waitpid(child_pid, &code, 0) < 0 )
			fatal("waitpid: %m");
		if ( fs->flags & FILESYSTEM_FLAG_FSCK_MUST )
		{
			if ( WIFSIGNALED(code) )
				fatal("%s: Mandatory repair failed: %s: %s", bdev_path,
				      fs->fsck, strsignal(WTERMSIG(code)));
			else if ( !WIFEXITED(code) )
				fatal("%s: Mandatory repair failed: %s: %s", bdev_path,
				      fs->fsck, "Unexpected unusual termination");
			else if ( WEXITSTATUS(code) == 127 )
				fatal("%s: Mandatory repair failed: %s: %s", bdev_path,
				      fs->fsck, "Filesystem checker is absent");
			else if ( WEXITSTATUS(code) & 2 )
				fatal("%s: Mandatory repair: %s: %s", bdev_path,
				      fs->fsck, "System reboot is necessary");
			else if ( WEXITSTATUS(code) != 0 && WEXITSTATUS(code) != 1 )
				fatal("%s: Mandatory repair failed: %s: %s", bdev_path,
				      fs->fsck, "Filesystem checker was unsuccessful");
		}
		else
		{
			if ( WIFSIGNALED(code) )
				fatal("%s: Filesystem check failed: %s: %s", bdev_path,
				      fs->fsck, strsignal(WTERMSIG(code)));
			else if ( !WIFEXITED(code) )
				fatal("%s: Filesystem check failed: %s: %s", bdev_path,
				      fs->fsck, "Unexpected unusual termination");
			else if ( WEXITSTATUS(code) == 127 )
				warning("%s: Skipping filesystem check: %s: %s", bdev_path,
				        fs->fsck, "Filesystem checker is absent");
			else if ( WEXITSTATUS(code) & 2 )
				fatal("%s: Filesystem check: %s: %s", bdev_path,
				      fs->fsck, "System reboot is necessary");
			else if ( WEXITSTATUS(code) != 0 && WEXITSTATUS(code) != 1 )
				fatal("%s: Filesystem check failed: %s: %s", bdev_path,
				     fs->fsck, "Filesystem checker was unsuccessful");
		}
	} while ( 0 );
	if ( !fs->driver )
		fatal("%s: Don't know how to mount a %s filesystem",
		      bdev_path, fs->fstype_name);
	chain_init_pid_by_filesystem = getpid();
	if ( atexit(init_chain_by_filesystem_unmount) != 0 )
		fatal("atexit: %m");
	if ( !(chain_mount_point = strdup("/tmp/fs.XXXXXX")) )
		fatal("strdup: %m");
	if ( !mkdtemp(chain_mount_point) )
		fatal("mkdtemp: %s: %m", "/tmp/fs.XXXXXX");
	struct stat st;
	if ( stat(chain_mount_point, &st) < 0 )
		fatal("stat: %s: %m", chain_mount_point);
	if ( (chain_mount_pid = fork()) < 0 )
		fatal("%s: Unable to mount: fork: %m", bdev_path);
	// TODO: This design is broken. The filesystem should tell us when it is
	//       ready instead of having to poll like this.
	if ( chain_mount_pid == 0 )
	{
		execlp(fs->driver, fs->driver, "--foreground", "--pretend-mount-path=/",
		       bdev_path, chain_mount_point, (const char*) NULL);
		warning("%s: Failed to load filesystem driver: %s: %m", bdev_path, fs->driver);
		_exit(127);
	}
	while ( true )
	{
		struct stat newst;
		if ( stat(chain_mount_point, &newst) < 0 )
			fatal("stat: %s: %m", chain_mount_point);
		if ( newst.st_dev != st.st_dev || newst.st_ino != st.st_ino )
			break;
		int code;
		pid_t child = waitpid(chain_mount_pid, &code, WNOHANG);
		if ( child < 0 )
			fatal("waitpid: %m");
		if ( child != 0 )
		{
			chain_mount_pid = -1;
			if ( WIFSIGNALED(code) )
				fatal("%s: Mount failed: %s: %s", bdev_path, fs->driver,
				      strsignal(WTERMSIG(code)));
			else if ( !WIFEXITED(code) )
				fatal("%s: Mount failed: %s: %s", bdev_path, fs->driver,
				      "Unexpected unusual termination");
			else if ( WEXITSTATUS(code) == 127 )
				fatal("%s: Mount failed: %s: %s", bdev_path, fs->driver,
				      "Filesystem driver is absent");
			else if ( WEXITSTATUS(code) == 0 )
				fatal("%s: Mount failed: %s: Unexpected successful exit",
				      bdev_path, fs->driver);
			else
				fatal("%s: Mount failed: %s: Exited with status %i", bdev_path,
				      fs->driver, WEXITSTATUS(code));
		}
		struct timespec delay = timespec_make(0, 50L * 1000L * 1000L);
		nanosleep(&delay, NULL);
	}
	int result = init_chain_by_path(chain_mount_point);
	init_chain_by_filesystem_unmount();
	return result;
}

int init_chain_by_uuid(const char* uuid)
{
	init_early();
	prepare_block_devices();
	if ( !uuid_validate(uuid) )
		fatal("`%s' is not a valid uuid", uuid);
	struct device_match match;
	memset(&match, 0, sizeof(match));
	search_by_uuid(uuid, ensure_single_device_match, &match);
	if ( !match.path )
		fatal("No devices matching uuid %s were found", uuid);
	if ( !match.bdev )
		fatal("Don't know which particular device to boot with uuid %s", uuid);
	assert(match.bdev->fs);
	return init_chain_by_filesystem(match.bdev->fs);
}

int init_chain_by_uuid_file(const char* path)
{
	// Find the uuid of the root filesystem.
	FILE* fp = fopen(path, "r");
	if ( !fp )
		fatal("%s: %m", path);
	char* uuid = read_single_line(fp);
	if ( !uuid )
		fatal("read: %s: %m", path);
	fclose(fp);
	if ( !uuid_validate(uuid) )
		fatal("%s: `%s' is not a valid uuid", path, uuid);
	int result = init_chain_by_uuid(uuid);
	free(uuid);
	return result;
}

int init_chain(const char* parameter)
{
	bool parameter_was_null = parameter == NULL;
	if ( !parameter )
	{
		if ( !setfsent() )
			fatal("/etc/fstab: %m");
		errno = 0;
		struct fstab* fs = getfsfile("/");
		if ( !fs && errno )
			fatal("/etc/fstab: %m");
		if ( !fs )
			fatal("/etc/fstab: Root filesystem not found in filesystem table");
		parameter = fs->fs_spec;
		if ( strncmp(parameter, "UUID=", strlen("UUID=")) == 0 )
			parameter += strlen("UUID=");
	}
	if ( uuid_validate(parameter) )
		return init_chain_by_uuid(parameter);
	struct stat st;
	if ( stat(parameter, &st) < 0 )
		error(2, errno, "%s", parameter);
	if ( S_ISDIR(st.st_mode) && !parameter_was_null )
		return init_chain_by_path(parameter);
	// TODO: Implement this case so it works in /etc/fstab.
	//else if ( S_ISBLK(st.st_mode) )
	//	return init_chain_by_device(parameter);
	else if ( S_ISREG(st.st_mode) && !parameter_was_null )
		return init_chain_by_uuid_file(parameter);
	else
		fatal("%s: Don't how how to chain initialize this", parameter);
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Initialize and manage the userland.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	const char* chain = NULL;
	const char* target = NULL;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strncmp(arg, "--chain=", strlen("--chain=")) )
			chain = arg + strlen("--chain=");
		else if ( !strcmp(arg, "--chain") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--chain' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			chain = argv[i+1];
			argv[++i] = NULL;
		}
		else if ( !strncmp(arg, "--target=", strlen("--target=")) )
			target = arg + strlen("--target=");
		else if ( !strcmp(arg, "--target") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--target' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			target = argv[i+1];
			argv[++i] = NULL;
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	char* target_string = NULL;
	if ( !target )
	{
		const char* target_path = "/etc/init/target";
		if ( chain )
			target = "chain";
		else if ( access(target_path, F_OK) == 0 )
		{
			FILE* target_fp = fopen(target_path, "r");
			if ( !target_fp )
				fatal("%s: %m", target_path);
			target_string = read_single_line(target_fp);
			if ( !target_string )
				fatal("read: %s: %m", target_path);
			target = target_string;
			fclose(target_fp);
		}
		else
			target = "single-user";
	}

	int result;
	if ( !strcmp(target, "chain") )
		return init_chain(chain);
	else if ( !strcmp(target, "single-user") ||
	          !strcmp(target, "multi-user") )
		return init(target);
	else
		fatal("Unknown initialization target `%s'", target);
	free(target_string);
	return result;
}
