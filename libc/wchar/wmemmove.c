/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * wchar/wmemmove.c
 * Copy wide memory between potentially overlapping regions.
 */

#include <stdint.h>
#include <wchar.h>

wchar_t* wmemmove(wchar_t* dst, const wchar_t* src, size_t n)
{
	if ( (uintptr_t) dst < (uintptr_t) src )
	{
		for ( size_t i = 0; i < n; i++ )
			dst[i] = src[i];
	}
	else
	{
		for ( size_t i = 0; i < n; i++ )
			dst[n-(i+1)] = src[n-(i+1)];
	}
	return dst;
}
