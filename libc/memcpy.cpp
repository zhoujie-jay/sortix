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

	memcpy.cpp
	Copy memory between non-overlapping regions.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

static void* memcpy_aligned(unsigned long* dest,
                            const unsigned long* src,
                            size_t length)
{
	size_t numcopies = length / sizeof(unsigned long);
	for ( size_t i = 0; i < numcopies; i++ )
		dest[i] = src[i];
	return dest;
}

static inline bool IsWordAligned(uintptr_t addr)
{
	const size_t WORDSIZE = sizeof(unsigned long);
	return (addr / WORDSIZE * WORDSIZE) == addr;
}

extern "C" void* memcpy(void* destptr, const void* srcptr, size_t length)
{
	if ( IsWordAligned((uintptr_t) destptr) &&
	     IsWordAligned((uintptr_t) srcptr) &&
	     IsWordAligned(length) )
	{
		unsigned long* dest = (unsigned long*) destptr;
		const unsigned long* src = (const unsigned long*) srcptr;
		return memcpy_aligned(dest, src, length);
	}
	uint8_t* dest = (uint8_t*) destptr;
	const uint8_t* src = (const uint8_t*) srcptr;
	for ( size_t i = 0; i < length; i += sizeof(uint8_t) )
	{
		dest[i] = src[i];
	}
	return dest;
}
