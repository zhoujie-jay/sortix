/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    dirent/dscandir_r.cpp
    Filtered and sorted directory reading.

*******************************************************************************/

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern "C"
int dscandir_r(DIR* dir,
               struct dirent*** namelist_ptr,
               int (*filter)(const struct dirent*, void*),
               void* filter_ctx,
               int (*compare)(const struct dirent**, const struct dirent**, void*),
               void* compare_ctx)
{
	rewinddir(dir);

	size_t namelist_used = 0;
	size_t namelist_length = 0;
	struct dirent** namelist = NULL;

	if ( false )
	{
	out_error:
		for ( size_t i = 0; i < namelist_used; i++ )
			free(namelist[i]);
		free(namelist);
		return errno = EOVERFLOW, -1;
	}

	while ( struct dirent* entry = readdir(dir) )
	{
		if ( filter && !filter(entry, filter_ctx) )
			continue;
		if ( (size_t) INT_MAX <= namelist_used )
			goto out_error;
		if ( namelist_used == namelist_length )
		{
			size_t new_length = namelist_length ? 2 * namelist_length : 8;
			size_t new_size = new_length * sizeof(struct dirent*);
			struct dirent** list = (struct dirent**) realloc(namelist, new_size);
			if ( !list )
				goto out_error;
			namelist = list;
			namelist_length = new_length;
		}
		size_t name_length = strlen(entry->d_name);
		size_t dirent_size = sizeof(struct dirent) + name_length + 1;
		struct dirent* dirent = (struct dirent*) malloc(dirent_size);
		if ( !dirent )
			goto out_error;
		memcpy(dirent, entry, sizeof(*entry));
		strcpy(dirent->d_name, entry->d_name);
		namelist[namelist_used++] = dirent;
	}

	if ( compare )
		qsort_r(namelist, namelist_used, sizeof(struct dirent*),
		      (int (*)(const void*, const void*, void*)) compare, compare_ctx);

	return *namelist_ptr = namelist, (int) namelist_used;
}
