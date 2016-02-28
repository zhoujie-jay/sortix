/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    wchar/wcsnrtombs.c
    Convert a wide-character string to multibyte string.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

size_t wcsnrtombs(char* restrict dst,
                  const wchar_t** restrict src_ptr,
                  size_t src_len,
                  size_t dst_len,
                  mbstate_t* restrict ps)
{
	static mbstate_t static_ps;
	if ( !ps )
		ps = &static_ps;

	assert(src_ptr && *src_ptr);
	const wchar_t* src = *src_ptr;

	// Continue to encode multibyte characters until we have filled the
	// destination buffer or if we have exhausted the limit on input wide chars.
	size_t dst_offset = 0;
	size_t src_offset = 0;
	while ( (!dst || dst_offset < dst_len) && src_offset < src_len )
	{
		mbstate_t ps_copy = *ps;
		wchar_t wc = src[src_offset];
		char mb[MB_CUR_MAX];
		size_t amount = wcrtomb(mb, wc, ps);

		// Stop in the event a decoding error occured.
		if ( amount == (size_t) -1 )
			return *src_ptr = src + src_offset, (size_t) -1;

		// Stop decoding early in the event we encountered a partial character,
		// or that we ran out of space in the destination buffer.
		if ( amount == (size_t) -2 || (dst && dst_offset - dst_len < amount ) )
		{
			*ps = ps_copy;
			break;
		}

		// Store the decoded multibyte character in the destination buffer.
		if ( dst )
			memcpy(dst + dst_offset, mb, amount);

		// Stop decoding after decoding a null character and return a NULL
		// source pointer to the caller, not including the null character in the
		// number of characters stored in the destination buffer.
		if ( wc == L'\0' )
		{
			src = NULL;
			src_offset = 0;
			break;
		}

		dst_offset += amount;
		src_offset++;
	}

	return *src_ptr = src + src_offset, dst_offset;
}
