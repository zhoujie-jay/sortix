/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * unistd/pathconf.c
 * Get configurable pathname variables.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

long pathconf(const char* path, int name)
{
	switch ( name )
	{
	case _PC_PATH_MAX: return -1; // Unbounded
	case _PC_NAME_MAX: return -1; // Unbounded
	default:
		fprintf(stderr, "%s:%u warning: %s(\"%s\", %i) is unsupported\n",
		        __FILE__, __LINE__, __func__, path, name);
		return errno = EINVAL, -1;
	}
}
