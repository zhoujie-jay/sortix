/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/setvbuf_unlocked.c
 * Sets up buffering semantics for a FILE.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int setvbuf_unlocked(FILE* fp, char* buf, int mode, size_t size)
{
	(void) buf;
	(void) size;
	if ( mode == -1 )
	{
#ifdef __is_sortix_libk
		mode = _IOLBF;
#else
		mode = _IOFBF;
		int saved_errno = errno;
		if ( isatty(fileno_unlocked(fp)) )
			mode = _IOLBF;
		errno = saved_errno;
#endif
	}
	if ( !fp->buffer )
		mode = _IONBF;
	fp->buffer_mode = mode;
	fp->flags |= _FILE_BUFFER_MODE_SET;
	fp->fflush_indirect = fflush_unlocked;
	return 0;
}
