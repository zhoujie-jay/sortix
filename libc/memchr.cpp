/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    memchr.cpp
    Scans memory for a character.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

extern "C" void* memchr(const void* s, int c, size_t size)
{
	const uint8_t* buf = (const uint8_t*) s;
	for ( size_t i = 0; i < size; i++ )
	{
		if ( buf[i] == c ) { return (void*) (buf + i); }
	}
	return NULL;
}
