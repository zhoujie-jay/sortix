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

    dirent/dir.cpp
    DIR* is an interface allowing various directory backends.

*******************************************************************************/

#include <sys/types.h>

#include <dirent.h>
#include <DIR.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static DIR* firstdir = NULL;

extern "C" void dregister(DIR* dir)
{
	dir->flags |= _DIR_REGISTERED;
	if ( !firstdir ) { firstdir = dir; return; }
	dir->next = firstdir;
	firstdir->prev = dir;
	firstdir = dir;
}

extern "C" void dunregister(DIR* dir)
{
	if ( !(dir->flags & _DIR_REGISTERED) )
		return;
	if ( !dir->prev )
		firstdir = dir->next;
	if ( dir->prev )
		dir->prev->next = dir->next;
	if ( dir->next )
		dir->next->prev = dir->prev;
	dir->flags &= ~_DIR_REGISTERED;
}

extern "C" struct dirent* readdir(DIR* dir)
{
	if ( !dir->read_func )
	{
		dir->flags |= _DIR_ERROR;
		errno = EBADF;
		return 0;
	}

	size_t size = dir->entrysize;
	int status = dir->read_func(dir->user, dir->entry, &size);
	if ( status < 0 )
	{
		dir->flags |= _DIR_ERROR;
		return NULL;
	}
	if ( 0 < status )
	{
		struct dirent* biggerdir = (struct dirent*) malloc(size);
		if ( !biggerdir )
		{
			dir->flags |= _DIR_ERROR;
			return NULL;
		}
		free(dir->entry);
		dir->entry = biggerdir;
		dir->entrysize = size;
		return readdir(dir);
	}

	dir->flags &= ~_DIR_ERROR;

	if ( !dir->entry->d_name[0] )
	{
		dir->flags |= _DIR_EOF;
		return NULL;
	}

	return dir->entry;
}

extern "C" int closedir(DIR* dir)
{
	int result = (dir->close_func) ? dir->close_func(dir->user) : 0;
	dunregister(dir);
	free(dir->entry);
	if ( dir->free_func )
		dir->free_func(dir);
	return result;
}

extern "C" void rewinddir(DIR* dir)
{
	if ( dir->rewind_func )
		dir->rewind_func(dir->user);
	dir->flags &= ~_DIR_EOF;
}

extern "C" int dirfd(DIR* dir)
{
	if ( !dir->fd_func )
		return errno = EBADF, 0;
	return dir->fd_func(dir->user);
}

extern "C" void dclearerr(DIR* dir)
{
	dir->flags &= ~_DIR_ERROR;
}

extern "C" int derror(DIR* dir)
{
	return dir->flags & _DIR_ERROR;
}

extern "C" int deof(DIR* dir)
{
	return dir->flags & _DIR_EOF;
}

static void dfreedir(DIR* dir)
{
	free(dir);
}

extern "C" DIR* dnewdir(void)
{
	DIR* dir = (DIR*) calloc(sizeof(DIR), 1);
	if ( !dir )
		return NULL;
	dir->flags = 0;
	dir->free_func = dfreedir;
	dregister(dir);
	return dir;
}

extern "C" int dcloseall(void)
{
	int result = 0;
	while ( firstdir )
		result |= closedir(firstdir);
	return result ? EOF : 0;
}
