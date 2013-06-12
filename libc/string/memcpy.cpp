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

    string/memcpy.cpp
    Copy memory between non-overlapping regions.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

#if 8 < __SIZEOF_LONG__
#error unsigned long is bigger than expected, please add support to this file.
#endif

inline static void* memcpy_slow(void* restrict dstptr,
                                const void* restrict srcptr, size_t length)
{
	uint8_t* restrict dst = (uint8_t* restrict) dstptr;
	const uint8_t* restrict src = (const uint8_t* restrict) srcptr;
	for ( size_t i = 0; i < length; i += sizeof(uint8_t) )
		dst[i] = src[i];
	return dstptr;
}

extern "C" void* memcpy(void* restrict dstptr, const void* restrict srcptr,
                        size_t length)
{
	const unsigned long unalignmask = sizeof(unsigned long) - 1;
	const unsigned long srcunalign = (unsigned long) srcptr & unalignmask;
	const unsigned long dstunalign = (unsigned long) dstptr & unalignmask;
	if ( srcunalign != dstunalign )
		return memcpy_slow(dstptr, srcptr, length);

	union
	{
		unsigned long srcval;
		const uint8_t* restrict src8;
		const uint16_t* restrict src16;
		const uint32_t* restrict src32;
		const uint64_t* restrict src64;
		const unsigned long* restrict srcul;
	};
	srcval = (unsigned long) srcptr;

	union
	{
		unsigned long dstval;
		uint8_t* restrict dst8;
		uint16_t* restrict dst16;
		uint32_t* restrict dst32;
		uint64_t* restrict dst64;
		unsigned long* restrict dstul;
	};
	dstval = (unsigned long) dstptr;

	if ( dstunalign )
	{
		if ( 1 <= length && !(dstval & (1-1)) && (dstval & (2-1)) )
			*dst8++ = *src8++,
			length -= 1;

		if ( 2 <= length && !(dstval & (2-1)) && (dstval & (4-1)) )
			*dst16++ = *src16++,
			length -= 2;

	#if 8 <= __SIZEOF_LONG__
		if ( 4 <= length && !(dstval & (4-1)) && (dstval & (8-1)) )
			*dst32++ = *src32++,
			length -= 4;
	#endif
	}

	size_t numcopies = length / sizeof(unsigned long);
#if 0
#if defined(__x86_64__) || defined(__i386__)
	unsigned long zeroed_numcopies;
#if defined(__x86_64__)
	asm volatile ("rep movsq" : "=c"(zeroed_numcopies), "=S"(srcul), "=D"(dstul)
	                          : "c"(numcopies), "S"(srcul), "D"(dstul)
	                          : "memory");
#elif defined(__i386__)
	asm volatile ("rep movsd" : "=c"(zeroed_numcopies), "=S"(srcul), "=D"(dstul)
	                          : "c"(numcopies), "S"(srcul), "D"(dstul)
	                          : "memory");
#endif
#endif
#else
	for ( size_t i = 0; i < numcopies; i++ )
		*dstul++ = *srcul++;
#endif

	length -= numcopies * sizeof(unsigned long);

	if ( length )
	{
	#if 8 <= __SIZEOF_LONG__
		if ( 4 <= length  )
			*dst32++ = *src32++,
			length -= 4;
	#endif

		if ( 2 <= length  )
			*dst16++ = *src16++,
			length -= 2;

		if ( 1 <= length  )
			*dst8++ = *src8++,
			length -= 1;
	}

	return dstptr;
}
