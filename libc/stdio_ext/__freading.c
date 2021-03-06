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
 * stdio_ext/__freading.c
 * Returns whether the last operation was a read or FILE is read only.
 */

#include <stdio.h>
#include <stdio_ext.h>

int __freading(FILE* fp)
{
	int result = 0;
	flockfile(fp);
	if ( fp->flags & _FILE_LAST_READ )
		result = 1;
	else if ( (fp->flags & _FILE_READABLE) && !(fp->flags & _FILE_WRITABLE) )
		result = 1;
	funlockfile(fp);
	return result;
}
