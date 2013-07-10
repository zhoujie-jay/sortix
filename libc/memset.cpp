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

    memset.cpp
    Initializes a region of memory to a byte value.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

extern "C" void* memset(void* destptr, int value, size_t length)
{
	uint8_t* dest = (uint8_t*) destptr;
	for ( size_t i = 0; i < length; i++ )
		dest[i] = value & 0xFF;
	return destptr;
}
