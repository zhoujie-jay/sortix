/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014, 2015.

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

#include <sys/readdirents.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

extern "C" struct dirent* readdir(DIR* dir)
{
	int old_errno = errno;
	struct dirent fallback;
	struct dirent* entry = dir->entry ? dir->entry : &fallback;
	size_t size = dir->entry ? dir->size : sizeof(fallback);
	ssize_t amount;
	while ( (amount = readdirents(dir->fd, entry, size)) < 0 )
	{
		if ( errno != ERANGE )
			return NULL;
		errno = old_errno;
		size_t needed = entry->d_reclen;
		free(dir->entry);
		dir->entry = NULL;
		struct dirent* new_dirent = (struct dirent*) malloc(needed);
		if ( !new_dirent )
			return NULL;
		entry = dir->entry = new_dirent;
		size = dir->size = needed;
	}
	if ( amount == 0 )
		return NULL;
	return dir->entry;
}
