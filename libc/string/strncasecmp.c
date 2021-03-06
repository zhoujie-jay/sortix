/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
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
 * string/strncasecmp.c
 * Compares a prefix of two strings ignoring case.
 */

#include <string.h>
#include <ctype.h>

int strncasecmp(const char* a, const char* b, size_t max_count)
{
	for ( size_t i = 0; i < max_count; i++ )
	{
		unsigned char ac = (unsigned char) tolower((unsigned char) a[i]);
		unsigned char bc = (unsigned char) tolower((unsigned char) b[i]);
		if ( ac == '\0' && bc == '\0' )
			return 0;
		if ( ac < bc )
			return -1;
		if ( ac > bc )
			return 1;
	}
	return 0;
}
