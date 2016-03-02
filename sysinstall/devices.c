/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * devices.c
 * Utility functions to handle devices, partitions, and filesystems.
 */

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <mount/blockdevice.h>
#include <mount/devices.h>
#include <mount/filesystem.h>
#include <mount/harddisk.h>
#include <mount/partition.h>
#include <mount/uuid.h>

#include "devices.h"

struct harddisk** hds;
size_t hds_count;

const char* path_of_blockdevice(struct blockdevice* bdev)
{
	return bdev->p ? bdev->p->path : bdev->hd->path;
}

const char* device_path_of_blockdevice(struct blockdevice* bdev)
{
	while ( bdev->p )
		bdev = bdev->p->parent_bdev;
	return bdev->hd->path;
}

void unscan_filesystem(struct blockdevice* bdev)
{
	if ( bdev->fs )
	{
		filesystem_release(bdev->fs);
		bdev->fs = NULL;
	}
}

void scan_filesystem(struct blockdevice* bdev)
{
	enum filesystem_error fserr = blockdevice_inspect_filesystem(&bdev->fs, bdev);
	if ( fserr == FILESYSTEM_ERROR_ABSENT ||
	     fserr == FILESYSTEM_ERROR_UNRECOGNIZED )
		return;
	if ( fserr != FILESYSTEM_ERROR_NONE )
		return; // TODO: Perhaps print an error here?
}

void unscan_device(struct harddisk* hd)
{
	if ( hd->bdev.pt )
	{
		for ( size_t i = 0; i < hd->bdev.pt->partitions_count; i++ )
			unscan_filesystem(&hd->bdev.pt->partitions[i]->bdev);
		partition_table_release(hd->bdev.pt);
		hd->bdev.pt = NULL;
	}
	if ( hd->bdev.fs )
		unscan_filesystem(&hd->bdev);
}

void scan_device(struct harddisk* hd)
{
	unscan_device(hd);
	struct blockdevice* bdev = &hd->bdev;
	enum partition_error parterr = blockdevice_get_partition_table(&bdev->pt, bdev);
	if ( parterr == PARTITION_ERROR_ABSENT ||
	     parterr == PARTITION_ERROR_UNRECOGNIZED )
	{
		scan_filesystem(bdev);
		return;
	}
	else if ( parterr == PARTITION_ERROR_ERRNO ||
	          parterr != PARTITION_ERROR_NONE )
		return; // TODO: Perhaps print an error here?
	for ( size_t i = 0; i < bdev->pt->partitions_count; i++ )
		scan_filesystem(&bdev->pt->partitions[i]->bdev);
}

void unscan_devices(void)
{
	for ( size_t i = 0; i < hds_count; i++ )
	{
		unscan_device(hds[i]);
		harddisk_close(hds[i]);
	}
	hds_count = 0;
	free(hds);
	hds = NULL;
}

void scan_devices(void)
{
	unscan_devices();
	if ( !devices_open_all(&hds, &hds_count) )
	{
		// TODO: How should callers deal with error conditions from here?
		warn("iterating devices");
	}
	for ( size_t i = 0; i < hds_count; i++ )
		scan_device(hds[i]);
}

struct filesystem* search_for_filesystem_by_uuid(const unsigned char* uuid)
{
	for ( size_t di = 0; di < hds_count; di++ )
	{
		struct blockdevice* dbdev = &hds[di]->bdev;
		if ( dbdev->fs )
		{
			if ( (dbdev->fs->flags & FILESYSTEM_FLAG_UUID) &&
			     memcmp(dbdev->fs->uuid, uuid, 16) == 0 )
				return dbdev->fs;
		}
		else if ( dbdev->pt )
		{
			for ( size_t pi = 0; pi < dbdev->pt->partitions_count; pi++ )
			{
				struct blockdevice* pbdev = &dbdev->pt->partitions[pi]->bdev;
				if ( !pbdev->fs )
					continue;
				if ( (pbdev->fs->flags & FILESYSTEM_FLAG_UUID) &&
					 memcmp(pbdev->fs->uuid, uuid, 16) == 0 )
					return pbdev->fs;
			}
		}
	}
	return NULL;
}

struct filesystem* search_for_filesystem_by_spec(const char* spec)
{
	if ( strncmp(spec, "UUID=", strlen("UUID=")) == 0 )
	{
		const char* uuid_string = spec + strlen("UUID=");
		if ( !uuid_validate(uuid_string) )
			return NULL;
		unsigned char uuid[16];
		uuid_from_string(uuid, uuid_string);
		return search_for_filesystem_by_uuid(uuid);
	}
	return NULL;
}

bool check_existing_systems(void)
{
	for ( size_t di = 0; di < hds_count; di++ )
	{
		struct blockdevice* dbdev = &hds[di]->bdev;
		if ( dbdev->fs )
			return true;
		else if ( dbdev->pt )
			return 1 <= dbdev->pt->partitions_count;
	}
	return false;
}

bool check_lacking_partition_table(void)
{
	for ( size_t di = 0; di < hds_count; di++ )
	{
		struct blockdevice* dbdev = &hds[di]->bdev;
		if ( dbdev->fs )
			continue;
		if ( !dbdev->pt )
			return true;
	}
	return false;
}

bool fsck(struct filesystem* fs)
{
	const char* bdev_path = path_of_blockdevice(fs->bdev);
	printf("%s: Repairing filesystem due to inconsistency...\n", bdev_path);
	assert(fs->fsck);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
	{
		warn("%s: Mandatory repair failed: fork", bdev_path);
		return false;
	}
	if ( child_pid == 0 )
	{
		execlp(fs->fsck, fs->fsck, "-fp", "--", bdev_path, (const char*) NULL);
		warn("%s: Failed to load filesystem checker: %s", bdev_path, fs->fsck);
		_Exit(127);
	}
	int code;
	if ( waitpid(child_pid, &code, 0) < 0 )
	{
		warn("waitpid");
		return false;
	}
	if ( WIFSIGNALED(code) )
		warnx("%s: Mandatory repair failed: %s: %s", bdev_path,
		      fs->fsck, strsignal(WTERMSIG(code)));
	else if ( !WIFEXITED(code) )
		warnx("%s: Mandatory repair failed: %s: %s", bdev_path,
		      fs->fsck, "Unexpected unusual termination");
	else if ( WEXITSTATUS(code) == 127 )
		warnx("%s: Mandatory repair failed: %s: %s", bdev_path,
		      fs->fsck, "Filesystem checker is absent");
	else if ( WEXITSTATUS(code) & 2 )
		warnx("%s: Mandatory repair: %s: %s", bdev_path,
		      fs->fsck, "System reboot is necessary");
	else if ( WEXITSTATUS(code) != 0 && WEXITSTATUS(code) != 1 )
		warnx("%s: Mandatory repair failed: %s: %s", bdev_path,
		      fs->fsck, "Filesystem checker was unsuccessful");
	else
		return true;
	return false;
}

static int sort_mountpoint(const void* a_ptr, const void* b_ptr)
{
	const struct mountpoint* a = (const struct mountpoint*) a_ptr;
	const struct mountpoint* b = (const struct mountpoint*) b_ptr;
	return strcmp(a->entry.fs_file, b->entry.fs_file);
}

void free_mountpoints(struct mountpoint* mnts, size_t mnts_count)
{
	for ( size_t i = 0; i < mnts_count; i++ )
	{
		free(mnts[i].entry_line);
		free(mnts[i].absolute);
	}
	free(mnts);
}

bool load_mountpoints(const char* fstab_path,
                      struct mountpoint** mountpoints_out,
                      size_t* mountpoints_used_out)
{
	FILE* fp = fopen(fstab_path, "r");
	if ( !fp )
		return false;
	struct mountpoint* mountpoints = NULL;
	size_t mountpoints_used = 0;
	size_t mountpoints_length = 0;
	char* line = NULL;
	size_t line_size;
	ssize_t line_length;
	while ( 0 < (errno = 0, line_length = getline(&line, &line_size, fp)) )
	{
		if ( line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		struct fstab fstabent;
		if ( !scanfsent(line, &fstabent) )
			continue;
		if ( mountpoints_used == mountpoints_length )
		{
			size_t new_length = 2 * mountpoints_length;
			if ( !new_length )
				new_length = 16;
			struct mountpoint* new_mountpoints = (struct mountpoint*)
				reallocarray(mountpoints, new_length, sizeof(struct mountpoint));
			if ( !new_mountpoints )
			{
				free_mountpoints(mountpoints, mountpoints_used);
				free(line);
				fclose(fp);
				return false;
			}
			mountpoints = new_mountpoints;
			mountpoints_length = new_length;
		}
		struct mountpoint* mountpoint = &mountpoints[mountpoints_used++];
		memcpy(&mountpoint->entry, &fstabent, sizeof(fstabent));
		mountpoint->entry_line = line;
		mountpoint->pid = -1;
		if ( !(mountpoint->absolute = strdup(mountpoint->entry.fs_file)) )
		{
			free_mountpoints(mountpoints, mountpoints_used);
			fclose(fp);
			return false;
		}
		line = NULL;
		line_size = 0;
	}
	bool failure = errno;
	free(line);
	fclose(fp);
	if ( failure )
	{
		free_mountpoints(mountpoints, mountpoints_used);
		return false;
	}
	qsort(mountpoints, mountpoints_used, sizeof(struct mountpoint),
	      sort_mountpoint);
	*mountpoints_out = mountpoints;
	*mountpoints_used_out = mountpoints_used;
	return true;
}

void mountpoint_mount(struct mountpoint* mountpoint)
{
	struct filesystem* fs = mountpoint->fs;
	// TODO: It would be ideal to get an exclusive lock so that no other
	//       processes have currently mounted that filesystem.
	struct blockdevice* bdev = fs->bdev;
	const char* bdev_path = bdev->p ? bdev->p->path : bdev->hd->path;
	assert(bdev_path);
	if ( fs->flags & FILESYSTEM_FLAG_FSCK_MUST && !fsck(fs) )
		errx(2, "Failed to fsck %s", bdev_path);
	if ( !fs->driver )
		errx(2, "%s: Don't know how to mount a %s filesystem",
		     bdev_path, fs->fstype_name);
	const char* pretend_where = mountpoint->entry.fs_file;
	const char* where = mountpoint->absolute;
	struct stat st;
	if ( stat(where, &st) < 0 )
		err(2, "stat: %s", where);
	if ( (mountpoint->pid = fork()) < 0 )
		err(2, "%s: Unable to mount: fork", bdev_path);
	// TODO: This design is broken. The filesystem should tell us when it is
	//       ready instead of having to poll like this.
	if ( mountpoint->pid == 0 )
	{
		execlp(fs->driver, fs->driver, "--foreground", bdev_path, where,
		       "--pretend-mount-path", pretend_where, (const char*) NULL);
		warn("%s: Failed to load filesystem driver: %s", bdev_path, fs->driver);
		_exit(127);
	}
	while ( true )
	{
		struct stat newst;
		if ( stat(where, &newst) < 0 )
		{
			warn("stat: %s", where);
			if ( unmount(where, 0) < 0 && errno != ENOMOUNT )
				warn("unmount: %s", where);
			else if ( errno == ENOMOUNT )
				kill(mountpoint->pid, SIGQUIT);
			int code;
			waitpid(mountpoint->pid, &code, 0);
			mountpoint->pid = -1;
			exit(2);
		}
		if ( newst.st_dev != st.st_dev || newst.st_ino != st.st_ino )
			break;
		int code;
		pid_t child = waitpid(mountpoint->pid, &code, WNOHANG);
		if ( child < 0 )
			err(2, "waitpid");
		if ( child != 0 )
		{
			mountpoint->pid = -1;
			if ( WIFSIGNALED(code) )
				err(2, "%s: Mount failed: %s: %s", bdev_path, fs->driver,
				       strsignal(WTERMSIG(code)));
			else if ( !WIFEXITED(code) )
				err(2, "%s: Mount failed: %s: %s", bdev_path, fs->driver,
				       "Unexpected unusual termination");
			else if ( WEXITSTATUS(code) == 127 )
				err(2, "%s: Mount failed: %s: %s", bdev_path, fs->driver,
				       "Filesystem driver is absent");
			else if ( WEXITSTATUS(code) == 0 )
				err(2, "%s: Mount failed: %s: Unexpected successful exit",
				       bdev_path, fs->driver);
			else
				err(2, "%s: Mount failed: %s: Exited with status %i", bdev_path,
				       fs->driver, WEXITSTATUS(code));
		}
		struct timespec delay = timespec_make(0, 50L * 1000L * 1000L);
		nanosleep(&delay, NULL);
	}
}

void mountpoint_unmount(struct mountpoint* mountpoint)
{
	if ( mountpoint->pid < 0 )
		return;
	if ( unmount(mountpoint->absolute, 0) < 0 && errno != ENOMOUNT )
		warn("unmount: %s", mountpoint->entry.fs_file);
	else if ( errno == ENOMOUNT )
		kill(mountpoint->pid, SIGQUIT);
	int code;
	if ( waitpid(mountpoint->pid, &code, 0) < 0 )
		warn("waitpid");
	mountpoint->pid = -1;
}
