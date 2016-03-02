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
 * stdio/fflush_stop_reading_unlocked.c
 * Resets the FILE to a consistent state ready for writing.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int fflush_stop_reading_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_READ) )
		return 0;

	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	int saved_errno = errno;
	off_t my_pos = -1;
	if ( fp->seek_func )
		my_pos = fp->seek_func(fp->user, 0, SEEK_CUR);
	errno = saved_errno;

	int ret = 0;
	if ( 0 <= my_pos )
	{
		size_t buffer_ahead = fp->amount_input_buffered - fp->offset_input_buffer;
		off_t expected_pos = (uintmax_t) my_pos < (uintmax_t) buffer_ahead ? 0 :
		                     my_pos - buffer_ahead;
		if ( 0 <= my_pos && fp->seek_func(fp->user, expected_pos, SEEK_SET) < 0 )
			fp->flags |= _FILE_STATUS_ERROR, ret = EOF;
	}

	fp->amount_input_buffered = fp->offset_input_buffer = 0;
	fp->flags &= ~_FILE_LAST_READ;

	return ret;
}
