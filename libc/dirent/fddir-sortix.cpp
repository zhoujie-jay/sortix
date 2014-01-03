/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    dirent/fddir-sortix.cpp
    Handles the file descriptor backend for the DIR* API on Sortix.

*******************************************************************************/

#include <sys/readdirents.h>

#include <assert.h>
#include <dirent.h>
#include <DIR.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct fddir_sortix_struct
{
	struct kernel_dirent* dirent;
	struct kernel_dirent* current;
	size_t direntsize;
	int fd;
} fddir_sortix_t;

static int fddir_sortix_readents(fddir_sortix_t* info)
{
	if ( !info->dirent )
	{
		// Allocate a buffer of at least sizeof(kernel_dirent).
		info->direntsize = sizeof(struct kernel_dirent) + 4UL;
		info->dirent = (struct kernel_dirent*) malloc(info->direntsize);
		if ( !info->dirent )
			return -1;
	}

	if ( readdirents(info->fd, info->dirent, info->direntsize) < 0 )
	{
		if ( errno != ERANGE )
			return -1;
		size_t newdirentsize = sizeof(struct kernel_dirent) + info->dirent->d_namlen + 1;
		if ( newdirentsize < info->direntsize )
			newdirentsize *= 2;
		struct kernel_dirent* newdirent = (struct kernel_dirent*) malloc(newdirentsize);
		if ( !newdirent )
			return -1;
		free(info->dirent);
		info->dirent = newdirent;
		info->direntsize = newdirentsize;
		return fddir_sortix_readents(info);
	}

	return 0;
}

static int fddir_sortix_read(void* user, struct dirent* dirent, size_t* size)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	if ( !info->current )
	{
		if ( fddir_sortix_readents(info) )
			return -1;
		info->current = info->dirent;
	}

	size_t provided = (user) ? *size : 0;
	size_t needed = sizeof(struct dirent) + info->current->d_namlen + 1;
	*size = needed;
	if ( provided < needed )
		return 1;

	dirent->d_ino = info->current->d_ino;
	dirent->d_reclen = needed;
	strcpy(dirent->d_name, info->current->d_name);

	info->current = kernel_dirent_next(info->current);

	return 0;
}

static int fddir_sortix_rewind(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	return lseek(info->fd, 0, SEEK_SET);
}

static int fddir_sortix_fd(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	return info->fd;
}

static int fddir_sortix_close(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	int result = close(info->fd);
	free(info->dirent);
	free(info);
	return result;
}

extern "C" DIR* fdopendir(int fd)
{
	fddir_sortix_t* info = (fddir_sortix_t*) calloc(sizeof(fddir_sortix_t), 1);
	if ( !info )
		return NULL;

	DIR* dir = dnewdir();
	if ( !dir ) { free(info); return NULL; }

	int old_dflags = fcntl(fd, F_GETFD);
	if ( 0 <= old_dflags )
		fcntl(fd, F_SETFD, old_dflags | O_CLOEXEC);

	info->fd = fd;

	dir->read_func = fddir_sortix_read;
	dir->rewind_func = fddir_sortix_rewind;
	dir->fd_func = fddir_sortix_fd;
	dir->close_func = fddir_sortix_close;
	dir->user = (void*) info;

	return dir;
}

extern "C" DIR* opendir(const char* path)
{
	int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if ( fd < 0 ) { return NULL; }
	DIR* dir = fdopendir(fd);
	if ( !dir ) { close(fd); return NULL; }
	return dir;
}
