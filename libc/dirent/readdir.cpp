/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    dirent/readdir.cpp
    Reads a directory entry from a directory stream into a DIR-specific buffer.

*******************************************************************************/

#include <dirent.h>
#include <DIR.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

extern "C" struct dirent* readdir(DIR* dir)
{
	int old_errno = errno;

	if ( !dir->read_func )
		return dir->flags |= _DIR_ERROR, errno = EBADF, (struct dirent*) NULL;

	size_t size = dir->entrysize;
	int status;
	while ( 0 < (status = dir->read_func(dir->user, dir->entry, &size)) )
	{
		struct dirent* biggerdir = (struct dirent*) malloc(size);
		if ( !biggerdir )
			return dir->flags |= _DIR_ERROR, (struct dirent*) NULL;
		free(dir->entry);
		dir->entry = biggerdir;
		dir->entrysize = size;
	}

	if ( status < 0 )
		return dir->flags |= _DIR_ERROR, (struct dirent*) NULL;

	dir->flags &= ~_DIR_ERROR;

	if ( !dir->entry->d_name[0] )
		return dir->flags |= _DIR_EOF, errno = old_errno, (struct dirent*) NULL;

	return dir->entry;
}
