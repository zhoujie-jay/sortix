/*
 * Copyright (c) 2011, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * dirent/opendir.c
 * Opens a stream for the directory specified by the path.
 */

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

DIR* opendir(const char* path)
{
	int fd = open(path, O_SEARCH | O_DIRECTORY | O_CLOEXEC);
	if ( fd < 0 )
		return NULL;
	DIR* dir = (DIR*) calloc(sizeof(DIR), 1);
	if ( !dir )
		return close(fd), (DIR*) NULL;
	dir->fd = fd;
	return dir;
}
