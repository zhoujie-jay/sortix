/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * wchar/wcsstr.c
 * Locate a wide substring.
 */

#include <stdbool.h>
#include <wchar.h>

// TODO: This simple and hacky implementation runs in O(N^2) even though this
// problem can be solved in O(N).
wchar_t* wcsstr(const wchar_t* restrict haystack,
                const wchar_t* restrict needle)
{
	if ( !needle[0] )
		return (wchar_t*) haystack;
	for ( size_t i = 0; haystack[i]; i++ )
	{
		bool diff = false;
		for ( size_t j = 0; needle[j]; j++ )
		{
			if ( (diff = haystack[i+j] != needle[j]) )
				break;
		}
		if ( diff )
			continue;
		return (wchar_t*) haystack + i;
	}
	return NULL;
}
