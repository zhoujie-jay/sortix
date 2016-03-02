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
 * stdio/fwrite_unlocked.c
 * Writes data to a FILE.
 */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

size_t fwrite_unlocked(const void* ptr,
                       size_t element_size,
                       size_t num_elements,
                       FILE* fp)
{
	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, -1;

	const unsigned char* buf = (const unsigned char*) ptr;
	size_t count = element_size * num_elements;
	if ( count == 0 )
		return num_elements;

	if ( fp->buffer_mode == _IONBF )
	{
		if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
			setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);
		if ( !fp->write_func )
			return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, 0;
		if ( fp->flags & _FILE_LAST_READ )
			fflush_stop_reading_unlocked(fp);
		fp->flags |= _FILE_LAST_WRITE;
		fp->flags &= ~_FILE_STATUS_EOF;
		size_t sofar = 0;
		while ( sofar < count )
		{
			size_t request = count - sofar;
			if ( (size_t) SSIZE_MAX < request )
				request = SSIZE_MAX;
			ssize_t amount = fp->write_func(fp->user, buf + sofar, request);
			if ( amount < 0 )
				return fp->flags |= _FILE_STATUS_ERROR, sofar / num_elements;
			if ( amount == 0 )
				return fp->flags |= _FILE_STATUS_EOF, sofar / num_elements;
			sofar += amount;
		}
		return sofar / element_size;
	}

	for ( size_t n = 0; n < num_elements; n++ )
	{
		size_t offset = n * element_size;
		for ( size_t i = 0; i < element_size; i++ )
		{
			size_t index = offset + i;
			if ( fputc_unlocked(buf[index], fp) == EOF )
				return n;
		}
	}

	return num_elements;
}
