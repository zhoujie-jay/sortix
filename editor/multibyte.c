/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    multibyte.c
    Conversion from multibyte strings to wide strings.

*******************************************************************************/

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
