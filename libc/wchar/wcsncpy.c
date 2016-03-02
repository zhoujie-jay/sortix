/*
 * Copyright (c) 2011, 2012, 2013 Jonas 'Sortie' Termansen.
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
 * wchar/wcsncpy.c
 * Copies a string into a fixed size buffer and returns dest.
 */

#include <wchar.h>

wchar_t* wcsncpy(wchar_t* dest, const wchar_t* src, size_t n)
{
	size_t i;
	for ( i = 0; i < n && src[i] != L'\0'; i++ )
		dest[i] = src[i];
	for ( ; i < n; i++ )
		dest[i] = L'\0';
	return dest;
}
