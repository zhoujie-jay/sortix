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

	fddir-sortix.c
	Handles the file descriptor backend for the DIR* API on Sortix.

*******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/readdirents.h>
#include <dirent.h>

typedef struct fddir_sortix_struct
{
	struct sortix_dirent* dirent;
	struct sortix_dirent* current;
	size_t direntsize;
	int fd;
} fddir_sortix_t;

int fddir_sortix_readents(fddir_sortix_t* info)
{
	if ( !info->dirent )
	{
		// Allocate a buffer of at least sizeof(sortix_dirent).
		info->direntsize = sizeof(struct sortix_dirent) + 4UL;
		info->dirent = malloc(info->direntsize);
		if ( !info->dirent ) { return -1; }
	}

	if ( readdirents(info->fd, info->dirent, info->direntsize) )
	{
		if ( errno != ERANGE ) { return -1; }
		size_t newdirentsize = info->dirent->d_namelen;
		struct sortix_dirent* newdirent = malloc(newdirentsize);
		if ( !newdirent ) { return -1; }
		free(info->dirent);
		info->dirent = newdirent;
		info->direntsize = newdirentsize;
		return fddir_sortix_readents(info);
	}

	return 0;
}

int fddir_sortix_read(void* user, struct dirent* dirent, size_t* size)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	if ( !info->current )
	{
		if ( fddir_sortix_readents(info) ) { return -1; }
		info->current = info->dirent;
	}

	size_t provided = (user) ? *size : 0;
	size_t needed = sizeof(struct dirent) + info->current->d_namelen + 1;
	*size = needed;
	if ( provided < needed ) { return 1; }

	dirent->d_reclen = needed;
	strcpy(dirent->d_name, info->current->d_name);

	info->current = info->current->d_next;

	return 0;
}

int fddir_sortix_rewind(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	return lseek(info->fd, SEEK_SET, 0);
}

int fddir_sortix_fd(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	return info->fd;
}

int fddir_sortix_close(void* user)
{
	fddir_sortix_t* info = (fddir_sortix_t*) user;
	int result = close(info->fd);
	free(info->dirent);
	free(info);
	return result;
}

DIR* fdopendir(int fd)
{
	fddir_sortix_t* info = calloc(sizeof(fddir_sortix_t), 1);
	if ( !info ) { return NULL; }

	DIR* dir = dnewdir();
	if ( !dir ) { free(info); return NULL; }

	// TODO: Possibly set O_CLOEXEC on fd, as that's what GNU/Linux does.

	info->fd = fd;

	dir->read_func = fddir_sortix_read;
	dir->rewind_func = fddir_sortix_rewind;
	dir->fd_func = fddir_sortix_fd;
	dir->close_func = fddir_sortix_close;
	dir->user = (void*) info;

	return dir;
}

DIR* opendir(const char* path)
{
	int fd = open(path, O_SEARCH | O_DIRECTORY | O_CLOEXEC);
	if ( fd < 0 ) { return NULL; }
	DIR* dir = fdopendir(fd);
	if ( !dir ) { close(fd); return NULL; }
	return dir;
}

