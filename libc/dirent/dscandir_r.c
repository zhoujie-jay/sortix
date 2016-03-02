/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * dirent/dscandir_r.c
 * Filtered and sorted directory reading.
 */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

	struct dirent* entry;
	while ( (entry = readdir(dir)) )
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
