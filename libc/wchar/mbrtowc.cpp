/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    wchar/mbrtowc.cpp
    Convert a multibyte sequence to a wide character.

*******************************************************************************/

#include <errno.h>
#include <stdint.h>
#include <wchar.h>

extern "C"
size_t mbrtowc(wchar_t* restrict pwc, const char* restrict s, size_t n,
               mbstate_t* restrict /*ps*/)
{
	if ( !s )
	{
		// TODO: Restore ps to initial state if currently valid.
		return 0;
	}
	uint8_t* buf = (uint8_t*) s;
	wchar_t ret = 0;
	size_t numbytes = 0;
	size_t sequence_len = 1;
	while ( numbytes < sequence_len )
	{
		if ( numbytes == n )
		{
			// TODO: Support restore through the mbstate_t!
			return (size_t) -2;
		}
		uint8_t b = buf[numbytes++];

		bool is_continuation = b >> (8-2) == 0b10;
		if ( 1 == numbytes && is_continuation )
			return errno = EILSEQ, (size_t) -1;
		if ( 2 <= numbytes && !is_continuation )
			return errno = EILSEQ, (size_t) -1;

		wchar_t new_bits;
		size_t new_bits_num;
		if ( b >> (8-1) == 0b0 )
			new_bits = b & 0b01111111,
			new_bits_num = 7,
			sequence_len = 1;
		else if ( b >> (8-2) == 0b10 )
			new_bits = b & 0b00111111,
			new_bits_num = 6,
			sequence_len = 2;
		else if ( b >> (8-3) == 0b110 )
			new_bits = b & 0b00011111,
			new_bits_num = 5,
			sequence_len = 3;
		else if ( b >> (8-4) == 0b1110 )
			new_bits = b & 0b00001111,
			new_bits_num = 4,
			sequence_len = 4;
		else if ( b >> (8-5) == 0b11110 )
			new_bits = b & 0b00000111,
			new_bits_num = 3,
			sequence_len = 5;
		else if ( b >> (8-6) == 0b111110 )
			new_bits = b & 0b00000011,
			new_bits_num = 2,
			sequence_len = 6;
		else if ( b >> (8-7) == 0b1111110 )
			new_bits = b & 0b00000001,
			new_bits_num = 1,
			sequence_len = 7;
		else
			return errno = EILSEQ, (size_t) -1;
		ret = ret >> new_bits_num | new_bits;
	}
	if ( !ret )
	{
		// TODO: Reset ps to initial state.
		return 0;
	}
	if ( (numbytes == 2 && ret <= 0x007F) ||
	     (numbytes == 3 && ret <= 0x07FF) ||
	     (numbytes == 4 && ret <= 0xFFFF) ||
	     (numbytes == 5 && ret <= 0x1FFFFF) ||
	     (numbytes == 6 && ret <= 0x3FFFFFF) )
		return errno = EILSEQ, (size_t) -1;
	if ( pwc )
		*pwc = ret;
	return numbytes;
}
