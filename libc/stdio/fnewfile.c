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
 * stdio/fnewfile.c
 * Allocates and initializes a simple FILE object ready for construction.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fnewfile_destroyer(void* user, FILE* fp)
{
	(void) user;
	free(fp);
}

FILE* fnewfile(void)
{
	FILE* fp = (FILE*) malloc(sizeof(FILE) + BUFSIZ);
	if ( !fp )
		return NULL;
	memset(fp, 0, sizeof(FILE));
	fp->buffer = (unsigned char*) (fp + 1);
	fp->free_user = NULL;
	fp->free_func = fnewfile_destroyer;
	fresetfile(fp);
	fregister(fp);
	return fp;
}
