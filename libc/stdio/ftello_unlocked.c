/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * stdio/ftello_unlocked.c
 * Returns the current offset of a FILE.
 */

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>

off_t ftello_unlocked(FILE* fp)
{
	if ( !fp->seek_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, -1;
	off_t offset = fp->seek_func(fp->user, 0, SEEK_CUR);
	if ( offset < 0 )
		return -1;
	off_t read_ahead = fp->amount_input_buffered - fp->offset_input_buffer;
	off_t write_behind = fp->amount_output_buffered;
	if ( offset < read_ahead + write_behind ) // Too much ungetc'ing.
		return 0;
	return offset - read_ahead + write_behind;
}
