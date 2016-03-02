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
 * stdio/fshutdown.c
 * Uninstalls the backend from a FILE so another can be reinstalled.
 */

#include <stdio.h>
#include <stdlib.h>

int fshutdown(FILE* fp)
{
	int ret = fp->fflush_indirect ? fp->fflush_indirect(fp) : 0;
	if ( ret )
	{
		/* TODO: How to report errors here? fclose may need us to return its
		         exact error value, for instance, as with popen/pclose. */;
	}
	ret = fp->close_func ? fp->close_func(fp->user) : ret;
	// Resetting the FILE here isn't needed in the case where fclose calls us,
	// but it's nice to zero it out anyway (avoiding stale) data, and it's a
	// feature when called by freopen that wishes to reuse the FILE. It also
	// means that the file is always in a consistent state.
	fresetfile(fp);
	return ret;
}
