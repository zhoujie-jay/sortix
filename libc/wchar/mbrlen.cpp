/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    wchar/mbrlen.cpp
    Determine number of bytes in next multibyte character.

*******************************************************************************/

#include <errno.h>
#include <string.h>
#include <wchar.h>

static size_t utf8_header_length(unsigned char uc)
{
	if ( (uc & 0b11000000) == 0b10000000 )
		return 0;
	if ( (uc & 0b10000000) == 0b00000000 )
		return 1;
	if ( (uc & 0b11100000) == 0b11000000 )
		return 2;
	if ( (uc & 0b11110000) == 0b11100000 )
		return 3;
	if ( (uc & 0b11111000) == 0b11110000 )
		return 4;
	if ( (uc & 0b11111100) == 0b11111000 )
		return 5;
	if ( (uc & 0b11111110) == 0b11111100 )
		return 6;
	return (size_t) -1;
}

// TODO: Use the shift state.
extern "C"
size_t mbrlen(const char* restrict s, size_t n, mbstate_t* restrict ps)
{
	size_t expected_length;

	for ( size_t i = 0; i < n; i++ )
	{
		unsigned char uc = (unsigned char) s[i];

		if ( i == 0 )
		{
			if ( !uc )
			{
				memset(ps, 0, sizeof(*ps));
				return 0;
			}

			if ( (expected_length = utf8_header_length(uc)) == (size_t) -1 )
				return errno = EILSEQ, (size_t) -1;

			// Check if we encounted an unexpected character claiming to be in
			// the middle of a UTF-8 multibyte sequence (10xxxxxx).
			if ( expected_length == 0 )
				// TODO: Should we play catch up with the partial sequence?
				return errno = EILSEQ, (size_t) -1;
		}

		// All non-header bytes should be of the form 10xxxxxx.
		if ( 0 < i && expected_length < n && (uc & 0b11000000) != 0b10000000 )
			return errno = EILSEQ, (size_t) -1;

		if ( i + 1 == expected_length )
			return i + 1;
	}

	return (size_t) -2;
}
