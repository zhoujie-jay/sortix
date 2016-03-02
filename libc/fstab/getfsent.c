/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * fstab/getfsent.c
 * Read filesystem table entry.
 */

#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>

struct fstab* getfsent(void)
{
	if ( !__fstab_file && !setfsent() )
		return NULL;
	static struct fstab fs;
	static char* line = NULL;
	static size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (line_length = getline(&line, &line_size, __fstab_file)) )
	{
		if ( line_length && line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		if ( scanfsent(line, &fs) )
			return &fs;
	}
	free(line);
	line = NULL;
	line_size = 0;
	return NULL;
}
