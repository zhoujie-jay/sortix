/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * stdio/fflush_stop_writing_unlocked.c
 * Resets the FILE to a consistent state so it is ready for reading.
 */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

int fflush_stop_writing_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_WRITE) )
		return 0;

	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !fp->write_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	int result = 0;

	size_t count = fp->amount_output_buffered;
	size_t sofar = 0;
	while ( sofar < count )
	{
		size_t request = count - sofar;
		if ( (size_t) SSIZE_MAX < request )
			request = SSIZE_MAX;
		ssize_t amount = fp->write_func(fp->user, fp->buffer + sofar, request);
		if ( amount < 0 )
		{
			fp->flags |= _FILE_STATUS_ERROR;
			result = EOF;
			break;
		}
		if ( amount == 0 )
		{
			fp->flags |= _FILE_STATUS_EOF;
			result = EOF;
			break;
		}
		sofar += amount;
	}

	fp->amount_output_buffered = 0;
	fp->flags &= ~_FILE_LAST_WRITE;

	return result;
}
