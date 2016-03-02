/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/fgetc_unlocked.c
 * Reads a single character from a FILE.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int fgetc_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);

	if ( fp->buffer_mode == _IONBF )
	{
		unsigned char c;
		if ( fread_unlocked(&c, sizeof(c), 1, fp) != 1 )
			return EOF;
		return (int) c;
	}

	if ( !fp->read_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing_unlocked(fp);

	fp->flags |= _FILE_LAST_READ;
	fp->flags &= ~_FILE_STATUS_EOF;

	if ( !(fp->offset_input_buffer < fp->amount_input_buffered) )
	{
		assert(fp->buffer && BUFSIZ);

		size_t pushback = _FILE_MAX_PUSHBACK;
		if ( BUFSIZ <= pushback )
			pushback = 0;
		size_t count = BUFSIZ - pushback;
		if ( (size_t) SSIZE_MAX < count )
			count = SSIZE_MAX;
		ssize_t numread = fp->read_func(fp->user, fp->buffer + pushback, count);
		if ( numread < 0 )
			return fp->flags |= _FILE_STATUS_ERROR, EOF;
		if ( numread == 0 )
			return fp->flags |= _FILE_STATUS_EOF, EOF;

		fp->offset_input_buffer = pushback;
		fp->amount_input_buffered = pushback + numread;
	}

	return fp->buffer[fp->offset_input_buffer++];
}
