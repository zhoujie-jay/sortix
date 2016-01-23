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

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/termmode.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <brand.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <fsmarshall.h>

char* read_single_line(FILE* fp)
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

__attribute__((format(printf, 1, 2)))
char* print_string(const char* format, ...)
{
	char* ret;
	va_list ap;
	va_start(ap, format);
	int status = vasprintf(&ret, format, ap);
	va_end(ap);
	assert(0 <= status);
	return ret;
}

char* join_paths(const char* a, const char* b)
{
	size_t a_len = strlen(a);
	bool has_slash = (a_len && a[a_len-1] == '/') || b[0] == '/';
	return has_slash ? print_string("%s%s", a, b)
	                 : print_string("%s/%s", a, b);
}

typedef struct
{
	char** strings;
	size_t length;
	size_t capacity;
} string_array_t;

string_array_t string_array_make()
{
	string_array_t sa;
	sa.strings = NULL;
	sa.length = sa.capacity = 0;
	return sa;
}

void string_array_reset(string_array_t* sa)
{
	for ( size_t i = 0; i < sa->length; i++ )
		free(sa->strings[i]);
	free(sa->strings);
	*sa = string_array_make();
}

size_t string_array_find(string_array_t* sa, const char* str)
{
	for ( size_t i = 0; i < sa->length; i++ )
		if ( !strcmp(sa->strings[i], str) )
			return i;
	return SIZE_MAX;
}

bool string_array_contains(string_array_t* sa, const char* str)
{
	return string_array_find(sa, str) != SIZE_MAX;
}

bool string_array_append(string_array_t* sa, const char* str)
{
	if ( sa->length == sa->capacity )
	{
		size_t new_capacity = sa->capacity ? sa->capacity * 2 : 8;
		size_t new_size = sizeof(char*) * new_capacity;
		char** new_strings = (char**) realloc(sa->strings, new_size);
		if ( !new_strings )
			return false;
		sa->strings = new_strings;
		sa->capacity = new_capacity;
	}
	char* copy = str ? strdup(str) : NULL;
	if ( str && !copy )
		return false;
	sa->strings[sa->length++] = copy;
	return true;
}

int child()
{
	pid_t init_pid = getppid();
	char init_pid_str[sizeof(pid_t)*3];
	snprintf(init_pid_str, sizeof(pid_t)*3, "%ju", (uintmax_t) init_pid);
	setenv("INIT_PID", init_pid_str, 1);

	setpgid(0, 0);

	sigset_t oldset, sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGTTOU);
	sigprocmask(SIG_BLOCK, &sigs, &oldset);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &oldset, NULL);

	unsigned int termmode = 0;
	gettermmode(0, &termmode);
	settermmode(0, termmode & ~TERMMODE_DISABLE);

	const char* default_shell = "sh";
	const char* default_home = "/root";
	const char* shell;
	const char* home;
	if ( struct passwd* passwd = getpwuid(getuid()) )
	{
		setenv("USERNAME", passwd->pw_name, 1);
		home = passwd->pw_dir[0] ? passwd->pw_dir : default_home;
		setenv("HOME", home, 1);
		shell = passwd->pw_shell[0] ? passwd->pw_shell : default_shell;
		setenv("SHELL", shell, 1);
	}
	else
	{
		setenv("USERNAME", "root", 1);
		setenv("HOME", home = default_home, 1);
		setenv("SHELL", shell = default_shell, 1);
	}

	chdir(home);

	const char* newargv[] = { shell, NULL };

	execvp(shell, (char* const*) newargv);
	error(0, errno, "%s", shell);

	return 2;
}

int runsystem()
{
	pid_t childpid = fork();
	if ( childpid < 0 )
		error(2, errno, "fork");

	if ( childpid )
	{
		int status;
		waitpid(childpid, &status, 0);
		// TODO: Use the proper macro!
		if ( 128 <= WEXITSTATUS(status) || WIFSIGNALED(status) )
		{
			printf("Looks like the system crashed, trying to bring it back up.\n");
			return runsystem();
		}
		return WEXITSTATUS(status);
	}

	exit(child());
}

int chain_boot_path(const char* path)
{
	// Run the next init program and restart it in case of a crash.
try_reboot_system:
	if ( pid_t child_pid = fork() )
	{
		int status;
		waitpid(child_pid, &status, 0);
		// TODO: Use the proper macro!
		if ( 128 <= WEXITSTATUS(status) || WIFSIGNALED(status) )
		{
			printf("Looks like the system crashed, trying to bring it back up.\n");
			goto try_reboot_system;
		}
		return WEXITSTATUS(status);
	}

	// Switch to the new root directory,
	chroot(path);
	chdir("/");

	const char* init_path = "/bin/init";
	execl(init_path, init_path, NULL);
	exit(127);
}

int init_emergency(int errnum, const char* format, ...)
{
	fprintf(stderr, "init: emergency: ");
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	if ( errnum )
		fprintf(stderr, ": %s", strerror(errnum));
	fprintf(stderr, "\n");

	fprintf(stderr, "init: Dropping you to an emergency shell.\n");
	fprintf(stderr, "init: Run `init' again when you have resolved the "
	                "situation to continue.\n");

	return runsystem();
}

void add_block_devices_to_string_array(const char* path, string_array_t* sa)
{
	DIR* dir = opendir(path);
	if ( !dir )
		return;
	while ( struct dirent* entry = readdir(dir) )
	{
		if ( entry->d_name[0] == '.' )
			continue;
		char* dev_path = join_paths(path, entry->d_name);
		struct stat st;
		if ( !(stat(dev_path, &st) == 0 &&
		       S_ISBLK(st.st_mode) &&
		       string_array_append(sa, dev_path)) )
			free(dev_path);
	}
	closedir(dir);
}

bool is_ext2_filesystem(const char* path, const char* uuid = NULL)
{
	if ( pid_t child_pid = fork() )
	{
		int exit_status;
		waitpid(child_pid, &exit_status, 0);
		return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0;
	}
	if ( uuid )
		execlp("extfs", "extfs", "--probe", "--test-uuid", uuid, path, NULL);
	else
		execlp("extfs", "extfs", "--probe", path, NULL);
	exit(127);
}

bool is_master_boot_record(const char* path)
{
	if ( pid_t child_pid = fork() )
	{
		int exit_status;
		waitpid(child_pid, &exit_status, 0);
		return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0;
	}
	execlp("mbrfs", "mbrfs", "--probe", path, NULL);
	exit(127);
}

bool create_master_boot_record_partitions(const char* path, string_array_t* sa)
{
	int pipe_fds[2];
	pipe(pipe_fds);
	if ( pid_t child_pid = fork() )
	{
		close(pipe_fds[1]);
		FILE* mbrfp = fdopen(pipe_fds[0], "r");
		while ( char* partition = read_single_line(mbrfp) )
		{
			if ( string_array_contains(sa, partition) ||
			     !string_array_append(sa, partition) )
				free(partition);
		}
		fclose(mbrfp);
		int exit_status;
		waitpid(child_pid, &exit_status, 0);
		return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0;
	}
	dup2(pipe_fds[1], 1);
	close(pipe_fds[0]);
	close(pipe_fds[1]);
	execlp("mbrfs", "mbrfs", path, NULL);
	exit(127);
}

int chain_boot_device(const char* dev_path)
{
	// Create a directory where we will mount the root filesystem.
	const char* mount_point = "/fs";
	const char* mount_point_dev = "/fs/dev";
	mkdir(mount_point, 0666);

	// Get information about the mount point before mounting.
	struct stat orig_st, new_st;
	stat(mount_point, &orig_st);

	// Spawn the filesystem server for the root filesystem.
	pid_t fs_pid = fork();
	if ( !fs_pid )
	{
		execlp("extfs", "extfs", "--foreground", dev_path, mount_point, NULL);
		exit(127);
	}

	// Wait for the filesystem server to come online.
	struct timespec mount_wait_ts = timespec_make(0, 50L * 1000L * 1000L);
	do nanosleep(&mount_wait_ts, NULL), stat(mount_point, &new_st);
	while ( new_st.st_ino == orig_st.st_ino && new_st.st_dev == orig_st.st_dev );

	// Create a device directory in the root filesystem.
	mkdir(mount_point_dev, 0666);

	// Mount the current device directory inside the new root filesystem.
	int old_dev_fd = open("/dev", O_DIRECTORY | O_RDONLY);
	int new_dev_fd = open(mount_point_dev, O_DIRECTORY | O_RDONLY);
	fsm_fsbind(old_dev_fd, new_dev_fd, 0);
	close(new_dev_fd);
	close(old_dev_fd);

	int ret = chain_boot_path(mount_point);

	int root_fd = open(mount_point, O_RDONLY);
	if ( 0 <= root_fd )
	{
		fsync(root_fd);
		close(root_fd);
	}

	unmount(mount_point, 0);

	int fs_exitstatus;
	waitpid(fs_pid, &fs_exitstatus, 0);

	if ( ret == 127 )
		return init_emergency(errno, "Unable to locate the next init program");
	return ret;
}

int chain_boot_uuid(const char* root_uuid)
{
	string_array_t block_devices = string_array_make();
	add_block_devices_to_string_array("/dev", &block_devices);

	string_array_t root_block_devices = string_array_make();

	// Scan through all the block devices and check for a filesystem with the
	// desired uuid while creating partitions if encountering partition tables.
	for ( size_t i = 0; i < block_devices.length; i++ )
	{
		const char* device_path = block_devices.strings[i];
		assert(device_path);
		if ( is_ext2_filesystem(device_path) )
		{
			if ( is_ext2_filesystem(device_path, root_uuid) )
				string_array_append(&root_block_devices, device_path);
		}
		else if ( is_master_boot_record(device_path) )
		{
			create_master_boot_record_partitions(device_path, &block_devices);
		}
	}

	string_array_reset(&block_devices);

	// Panic if we are unable to locate the desired root filesystem.
	if ( !root_block_devices.length )
		return init_emergency(0, "Unable to locate root filesystem with uuid="
		                         "`%s'", root_uuid);

	// If we only found a single matching filesystem, we can just boot it.
	if ( root_block_devices.length == 1 )
		return chain_boot_device(root_block_devices.strings[0]);

	// Handle the case where multiple root filesystems with the correct uuid is
	// found - we have to ask the user for help in this case.
	fprintf(stderr, "init: Found multiple devices with uuid=`%s'.\n", root_uuid);
	fprintf(stderr, "init: Select the correct boot device or nothing to get an emergency shell.\n");
retry_ask_root_block_device:
	for ( size_t i = 0; i < root_block_devices.length; i++ )
		fprintf(stderr, "%zu.\t%s\n", i, root_block_devices.strings[i]);
	printf("Enter index or name of boot device [root shell]: ");
	fflush(stdout);

	char* input = read_single_line(stdin);
	if ( !input )
		return init_emergency(errno, "Unable read line from standard input");

	if ( !input[0] )
		return init_emergency(0, "ambigious root filesystem - shell selected");

	char* input_end;
	unsigned long index = strtoul(input, &input_end, 0);
	if ( *input_end )
	{
		if ( string_array_contains(&root_block_devices, input) )
			return chain_boot_device(input);
		fprintf(stderr, "init: error: `%s' is not an allowed choice\n", input);
		goto retry_ask_root_block_device;
	}

	if ( root_block_devices.length <= index )
	{
		fprintf(stderr, "init: error: `%lu' is not an allowed choice\n", index);
		goto retry_ask_root_block_device;
	}

	return chain_boot_device(root_block_devices.strings[index]);
}

void set_hostname()
{
	FILE* hostname_fp = fopen("/etc/hostname", "r");
	if ( !hostname_fp )
	{
		if ( errno == ENOENT )
			return;
		error(0, errno, "unable to open /etc/hostname, hostname is not set");
		return;
	}

	char* hostname = read_single_line(hostname_fp);
	if ( !hostname )
	{
		error(0, errno, "unable to read /etc/hostname, hostname is not set");
		fclose(hostname_fp);
		return;
	}

	fclose(hostname_fp);

	if ( sethostname(hostname, strlen(hostname) + 1) < 0 )
	{
		error(0, errno, "unable to set hostname to `%s'", hostname);
		free(hostname);
		return;
	}

	free(hostname);
}

void set_kblayout()
{
	FILE* kblayout_fp = fopen("/etc/kblayout", "r");
	if ( !kblayout_fp )
	{
		if ( errno == ENOENT )
			return;
		error(0, errno, "unable to open /etc/kblayout, keyboard layout is not set");
		return;
	}

	char* kblayout = read_single_line(kblayout_fp);
	if ( !kblayout )
	{
		error(0, errno, "unable to read /etc/kblayout, keyboard layout is not set");
		fclose(kblayout_fp);
		return;
	}

	fclose(kblayout_fp);

	pid_t child_pid = fork();
	if ( child_pid < 0 )
	{
		error(0, errno, "setting keyboard layout: fork");
		free(kblayout);
		return;
	}

	if ( !child_pid )
	{
		execlp("chkblayout", "chkblayout", "--", kblayout, (char*) NULL);
		error(0, errno, "setting keyboard layout: exec: chkblayout");
		_exit(127);
	}

	int status;
	waitpid(child_pid, &status, 0);

	free(kblayout);
}

int main(int argc, char* argv[])
{
	if ( 3 <= argc && !strcmp(argv[1], "--chain") )
		return chain_boot_device(argv[2]);

	// Set the default file creation mask.
	umask(022);

	// Set up the PATH variable.
	setenv("PATH", "/bin:/sbin", 1);

	// Set the terminal type.
	setenv("TERM", "sortix", 1);

	// Make sure that we have a /tmp directory.
	mkdir("/tmp", 01777);

	// Set the hostname as found in /etc/hostname.
	set_hostname();

	// Set the keyboard layout as found in /etc/kblayout.
	set_kblayout();

	// Find the uuid of the root filesystem.
	const char* root_uuid_file = "/etc/init/rootfs.uuid";
	FILE* root_uuid_fp = fopen(root_uuid_file, "r");

	// If there is no uuid of the root filesystem, the current root filesystem
	// is the real and final root filesystem and we boot it.
	if ( !root_uuid_fp && errno == ENOENT )
		return runsystem();

	if ( !root_uuid_fp )
		init_emergency(errno, "unable to open: `%s'", root_uuid_file);

	char* root_uuid = read_single_line(root_uuid_fp);
	if ( !root_uuid )
		init_emergency(errno, "unable to read: `%s'", root_uuid_file);

	fclose(root_uuid_fp);

	return chain_boot_uuid(root_uuid);
}
