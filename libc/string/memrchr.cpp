/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    string/memrchr.cpp
    Scans memory in reverse directory for a character.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

extern "C" void* memrchr(const void* ptr, int c, size_t n)
{
	const unsigned char* buf = (const unsigned char*) ptr;
	for ( size_t i = n; i != 0; i-- )
		if ( buf[i-1] == (unsigned char) c )
			return (void*) (buf + i-1);
	return NULL;
}
