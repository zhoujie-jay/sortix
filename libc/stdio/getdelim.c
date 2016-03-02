/*
 * Copyright (c) 2011, 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/getdelim.c
 * Delimited string input.
 */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t DEFAULT_BUFSIZE = 32UL;

ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* fp)
{
	int err = EINVAL;
	flockfile(fp);
	if ( !lineptr || !n )
	{
	failure:
		fp->flags |= _FILE_STATUS_ERROR;
		funlockfile(fp);
		return errno = err, -1;
	}
	err = ENOMEM;
	if ( !*lineptr )
		*n = 0;
	size_t written = 0;
	int c;
	do
	{
		if ( written == (size_t) SSIZE_MAX )
			goto failure;
		if ( *n <= (size_t) written + 1UL )
		{
			size_t newbufsize = *n ? 2UL * *n : DEFAULT_BUFSIZE;
			char* newbuf = (char*) realloc(*lineptr, newbufsize);
			if ( !newbuf )
				goto failure;
			*lineptr = newbuf;
			*n = newbufsize;
		}
		if ( (c = getc_unlocked(fp)) == EOF )
		{
			if ( !written || feof_unlocked(fp) )
			{
				funlockfile(fp);
				return -1;
			}
		}
		(*lineptr)[written++] = c;
	} while ( c != delim );
	funlockfile(fp);
	(*lineptr)[written] = 0;
	return (ssize_t) written;
}
