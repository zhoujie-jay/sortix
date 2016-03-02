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
 * unistd/ttyname.c
 * Returns the pathname of a terminal.
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

char* ttyname(int fd)
{
	static char* result = NULL;
	static size_t result_size = 0;
	while ( ttyname_r(fd, result, result_size) < 0 )
	{
		if ( errno != ERANGE )
			return NULL;
		size_t new_result_size = result_size ? 2 * result_size : 16;
		char* new_result = (char*) realloc(result, new_result_size);
		if ( !new_result )
			return NULL;
		result = new_result;
		result_size = new_result_size;
	}
	return result;
}
