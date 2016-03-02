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
 * dirent/scandir.c
 * Filtered and sorted directory reading.
 */

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int wrap_filter(const struct dirent* dirent, void* function)
{
	return ((int (*)(const struct dirent*)) function)(dirent);
}

static int wrap_compare(const struct dirent** dirent_a,
                        const struct dirent** dirent_b, void* function)
{
	return ((int (*)(const struct dirent**, const struct dirent**)) function)(dirent_a, dirent_b);
}

int scandir(const char* path, struct dirent*** namelist_ptr,
            int (*filter)(const struct dirent*),
            int (*compare)(const struct dirent**, const struct dirent**))
{
	DIR* dir = opendir(path);
	if ( !dir )
		return -1;
	int (*used_filter)(const struct dirent*,
	                   void*) = filter ? wrap_filter : NULL;
	int (*used_compare)(const struct dirent**,
	                    const struct dirent**,
	                    void*) = compare ? wrap_compare : NULL;
	int result = dscandir_r(dir, namelist_ptr,
	                        used_filter, (void*) filter,
	                        used_compare, (void*) compare);
	closedir(dir);
	return result;
}
