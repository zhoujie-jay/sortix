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
 * stdio/fputc_unlocked.c
 * Writes a character to a FILE.
 */

#include <errno.h>
#include <stdio.h>

int fputc_unlocked(int c, FILE* fp)
{
	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);

	if ( fp->buffer_mode == _IONBF )
	{
		unsigned char c_char = c;
		if ( fwrite_unlocked(&c_char, sizeof(c_char), 1, fp) != 1 )
			return EOF;
		return c;
	}

	if ( !fp->write_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( fp->flags & _FILE_LAST_READ )
		fflush_stop_reading_unlocked(fp);

	fp->flags |= _FILE_LAST_WRITE;
	fp->flags &= ~_FILE_STATUS_EOF;

	if ( fp->amount_output_buffered == BUFSIZ )
	{
		if ( fflush_unlocked(fp) == EOF )
			return EOF;
	}

	fp->buffer[fp->amount_output_buffered++] = c;

	if ( fp->buffer_mode == _IOLBF && c == '\n' )
	{
		if ( fflush_unlocked(fp) == EOF )
			return EOF;
	}

	return c;
}
