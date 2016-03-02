/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * stdlib/bsearch.c
 * Binary search.
 */

#include <stdint.h>
#include <stdlib.h>

void* bsearch(const void* key, const void* base, size_t nmemb,
              size_t size, int (*compare)(const void*, const void*))
{
	const uint8_t* baseptr = (const uint8_t*) base;
	// TODO: Just a quick and surprisingly correct yet slow implementation.
	for ( size_t i = 0; i < nmemb; i++ )
	{
		const void* candidate = baseptr + i * size;
		if ( !compare(key, candidate) )
			return (void*) candidate;
	}
	return NULL;
}
