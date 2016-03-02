/*
 * Copyright (c) 2011, 2012, 2013, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/fflush.c
 * Flushes a FILE.
 */

#include <pthread.h>
#include <stdio.h>

static FILE* volatile dummy_file = NULL;
weak_alias(dummy_file, __stdout_used);

int fflush(FILE* fp)
{
	if ( !fp )
	{
		int result = 0;
		pthread_mutex_lock(&__first_file_lock);
		if ( __stdout_used )
			fflush(__stdout_used);
		for ( fp = __first_file; fp; fp = fp->next )
		{
			flockfile(fp);
			if ( fp->flags & _FILE_LAST_WRITE )
				result |= fflush_unlocked(fp);
			funlockfile(fp);
		}
		pthread_mutex_unlock(&__first_file_lock);
		return result;
	}

	flockfile(fp);
	int ret = fflush_unlocked(fp);
	funlockfile(fp);
	return ret;
}
