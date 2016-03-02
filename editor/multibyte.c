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
 * multibyte.c
 * Conversion from multibyte strings to wide strings.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "multibyte.h"

wchar_t* convert_mbs_to_wcs(const char* mbs)
{
	if ( !mbs )
		mbs = "";

	size_t mbs_offset = 0;
	size_t mbs_length = strlen(mbs) + 1;

	mbstate_t ps;

	// Determinal the length of the resulting string.
	size_t wcs_length = 0;
	memset(&ps, 0, sizeof(ps));
	while ( true )
	{
		wchar_t wc;
		size_t count = mbrtowc(&wc, mbs + mbs_offset, mbs_length - mbs_offset, &ps);
		if ( count == (size_t) 0 )
			break;
		wcs_length++;
		if ( count == (size_t) -1 )
		{
			memset(&ps, 0, sizeof(ps));
			mbs_offset++; // Attempt to recover.
			continue;
		}
		if ( count == (size_t) -2 )
			break;
		mbs_offset += count;
	}

	wchar_t* result = (wchar_t*) malloc(sizeof(wchar_t) * (wcs_length + 1));
	if ( !result )
		return NULL;

	// Create the resulting string.
	mbs_offset = 0;
	size_t wcs_offset = 0;
	memset(&ps, 0, sizeof(ps));
	while ( true )
	{
		wchar_t wc;
		size_t count = mbrtowc(&wc, mbs + mbs_offset, mbs_length - mbs_offset, &ps);
		if ( count == (size_t) 0 )
			break;
		assert(mbs_offset < wcs_length);
		if ( count == (size_t) -1 )
		{
			memset(&ps, 0, sizeof(ps));
			result[wcs_offset++] = L'�';
			mbs_offset++; // Attempt to recover.
			continue;
		}
		if ( count == (size_t) -2 )
		{
			result[wcs_offset++] = L'�';
			break;
		}
		result[wcs_offset++] = wc;
		mbs_offset += count;
	}

	result[wcs_offset] = L'\0';

	return result;
}

char* convert_wcs_to_mbs(const wchar_t* wcs)
{
	const char* replacement_mb = "�";
	size_t replacement_mblen = strlen(replacement_mb);

	if ( !wcs )
		wcs = L"";

	mbstate_t ps;

	// Determinal the length of the resulting string.
	size_t wcs_offset = 0;
	size_t mbs_length = 0;
	memset(&ps, 0, sizeof(ps));
	while ( true )
	{
		wchar_t wc = wcs[wcs_offset++];
		char mb[MB_CUR_MAX];
		size_t count = wcrtomb(mb, wc, &ps);
		if ( count == (size_t) -1 )
		{
			memset(&ps, 0, sizeof(ps));
			mbs_length += replacement_mblen;
			continue;
		}
		mbs_length += count;
		if ( wc == L'\0' )
			break;
	}

	char* result = (char*) malloc(sizeof(char) * mbs_length);
	if ( !result )
		return NULL;

	// Create the resulting string.
	wcs_offset = 0;
	size_t mbs_offset = 0;
	memset(&ps, 0, sizeof(ps));
	while ( true )
	{
		wchar_t wc = wcs[wcs_offset++];
		char mb[MB_CUR_MAX];
		size_t count = wcrtomb(mb, wc, &ps);
		if ( count == (size_t) -1 )
		{
			memset(&ps, 0, sizeof(ps));
			assert(replacement_mblen <= mbs_length - mbs_offset);
			memcpy(result + mbs_offset, replacement_mb, sizeof(char) * replacement_mblen);
			mbs_offset += replacement_mblen;
			continue;
		}
		assert(count <= mbs_length - mbs_offset);
		memcpy(result + mbs_offset, mb, sizeof(char) * count);
		mbs_offset += count;
		if ( wc == L'\0' )
			break;
	}

	return result;
}
