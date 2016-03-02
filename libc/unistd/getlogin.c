/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * unistd/getlogin.c
 * Get name of user logged in at the controlling terminal.
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char* getlogin(void)
{
	static char* buffer = NULL;
	static size_t buffer_size = 0;

	int ret;
	while ( !buffer_size ||
	        ((ret = getlogin_r(buffer, buffer_size)) < 0 && errno == ERANGE) )
	{
		size_t new_buffer_size = buffer_size ? buffer_size * 2 : 64;
		char* new_buffer = (char*) realloc(buffer, new_buffer_size);
		if ( !new_buffer )
			return NULL;
		buffer = new_buffer;
		buffer_size = new_buffer_size;
	}

	return ret == 0 ? buffer : NULL;
}
