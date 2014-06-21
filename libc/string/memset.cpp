/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    string/memset.cpp
    Initializes a region of memory to a byte value.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

#include <__/wordsize.h>

extern "C" void* memset(void* dest_ptr, int value, size_t length)
{
	unsigned char* dest = (unsigned char*) dest_ptr;
#if __WORDSIZE == 32 || __WORDSIZE == 64
	if ( ((uintptr_t) dest_ptr & (sizeof(unsigned long) - 1)) == 0 )
	{
		unsigned long* ulong_dest = (unsigned long*) dest;
		size_t ulong_length = length / sizeof(unsigned long);
#if __WORDSIZE == 32
		unsigned long ulong_value =
			(unsigned long) ((unsigned char) value) << 0 |
			(unsigned long) ((unsigned char) value) << 8 |
			(unsigned long) ((unsigned char) value) << 16 |
			(unsigned long) ((unsigned char) value) << 24;
#elif __WORDSIZE == 64
		unsigned long ulong_value =
			(unsigned long) ((unsigned char) value) << 0 |
			(unsigned long) ((unsigned char) value) << 8 |
			(unsigned long) ((unsigned char) value) << 16 |
			(unsigned long) ((unsigned char) value) << 24 |
			(unsigned long) ((unsigned char) value) << 32 |
			(unsigned long) ((unsigned char) value) << 40 |
			(unsigned long) ((unsigned char) value) << 48 |
			(unsigned long) ((unsigned char) value) << 56;
#endif
		for ( size_t i = 0; i < ulong_length; i++ )
			ulong_dest[i] = ulong_value;
		dest += ulong_length * sizeof(unsigned long);
		length -= ulong_length * sizeof(unsigned long);
	}
#endif
	for ( size_t i = 0; i < length; i++ )
		dest[i] = (unsigned char) value;
	return dest_ptr;
}
