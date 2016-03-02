/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
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
 * stdio/fgets_unlocked.c
 * Reads a string from a FILE.
 */

#include <stdio.h>
#include <errno.h>

char* fgets_unlocked(char* restrict dest, int size, FILE* restrict fp)
{
	if ( size <= 0 )
		return errno = EINVAL, (char*) NULL;
	int i;
	for ( i = 0; i < size-1; i++ )
	{
		int c = fgetc_unlocked(fp);
		if ( c == EOF )
			break;
		dest[i] = c;
		if ( c == '\n' )
		{
			i++;
			break;
		}
	}
	if ( !i && (ferror_unlocked(fp) || feof_unlocked(fp)) )
		return NULL;
	// TODO: The end-of-file state is lost here if feof(fp) and we are reading
	//       from a terminal that encountered a soft EOF.
	dest[i] = '\0';
	return dest;
}
