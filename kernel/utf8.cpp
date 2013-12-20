/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    utf8.cpp
    Encodes UTF-32 strings in UTF-8.

*******************************************************************************/

#include <errno.h>

#include <sortix/kernel/kernel.h>

#include "utf8.h"

namespace Sortix
{
	namespace UTF8
	{
		unsigned Encode(uint32_t unicode, char* dest)
		{
			uint8_t* buf = (uint8_t*) dest;
			unsigned bytes = 1;
			unsigned bits = 7;
			if ( (1U<<7U) <= unicode ) { bytes = 2; bits = 11; }
			if ( (1U<<11U) <= unicode ) { bytes = 3; bits = 16; }
			if ( (1U<<16U) <= unicode ) { bytes = 4; bits = 21; }
			if ( (1U<<21U) <= unicode ) { bytes = 5; bits = 26; }
			if ( (1U<<26U) <= unicode ) { bytes = 6; bits = 31; }
			if ( (1U<<31U) <= unicode ) { errno = EINVAL; return 0; }

			uint8_t prefix;
			unsigned prefixavai;
			switch ( bytes )
			{
			case 1: prefixavai = 7; prefix = 0b0U << prefixavai; break;
			case 2: prefixavai = 5; prefix = 0b110U << prefixavai; break;
			case 3: prefixavai = 4; prefix = 0b1110U << prefixavai; break;
			case 4: prefixavai = 3; prefix = 0b11110U << prefixavai; break;
			case 5: prefixavai = 2; prefix = 0b111110U << prefixavai; break;
			case 6: prefixavai = 1; prefix = 0b1111110U << prefixavai; break;
			}

			// Put the first bits in the unused area of the prefix.
			prefix |= unicode >> (bits - prefixavai);
			*buf++ = prefix;
			unsigned bitsleft = bits - prefixavai;

			while ( bitsleft )
			{
				bitsleft -= 6;
				uint8_t elembits = (unicode>>bitsleft) & ((1U<<6U)-1U);
				uint8_t elem = (0b10U<<6U) | elembits;
				*buf++ = elem;
			}

			return bytes;
		}
	}
}
