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
 * stdio/ungetc_unlocked.c
 * Inserts data in front of the current read queue, allowing applications to
 * peek the incoming data and pretend they didn't.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

int ungetc_unlocked(int c, FILE* fp)
{
	if ( c == EOF )
		return EOF;

	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing_unlocked(fp);

	fp->flags |= _FILE_LAST_READ;

	// TODO: Is this a bug that ungetc doesn't work for unbuffered files?
	if ( fp->buffer_mode == _IONBF )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( fp->offset_input_buffer == 0 )
	{
		size_t amount = fp->amount_input_buffered - fp->offset_input_buffer;
		size_t offset = BUFSIZ - amount;
		if ( !offset )
			return EOF;
		memmove(fp->buffer + offset, fp->buffer, sizeof(fp->buffer[0]) * amount);
		fp->offset_input_buffer = offset;
		fp->amount_input_buffered = offset + amount;
	}

	fp->buffer[--fp->offset_input_buffer] = c;

	fp->flags &= ~_FILE_STATUS_EOF;

	return c;
}
