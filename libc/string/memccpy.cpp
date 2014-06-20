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

    string/memccpy.cpp
    Copy memory until length is met or character is encountered.

*******************************************************************************/

#include <stddef.h>
#include <string.h>

extern "C" void* memccpy(void* dest_ptr, const void* src_ptr, int c, size_t n)
{
	unsigned char* dest = (unsigned char*) dest_ptr;
	const unsigned char* src = (const unsigned char*) src_ptr;
	for ( size_t i = 0; i < n; i++ )
	{
		if ( (dest[i] = src[i]) == (unsigned char) c )
			return dest + i + 1;
	}
	return NULL;
}
