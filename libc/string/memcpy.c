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

    string/memcpy.c
    Copy memory between non-overlapping regions.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

inline static
void* memcpy_slow(void* restrict dst_ptr,
                  const void* restrict src_ptr,
                  size_t size)
{
	unsigned char* restrict dst = (unsigned char* restrict) dst_ptr;
	const unsigned char* restrict src = (const unsigned char* restrict) src_ptr;
	for ( size_t i = 0; i < size; i++ )
		dst[i] = src[i];
	return dst_ptr;
}

void* memcpy(void* restrict dst_ptr,
             const void* restrict src_ptr,
             size_t size)
{
#if 8 < __SIZEOF_LONG__
#warning "you should add support for your unexpectedly large unsigned long."
	return memcpy_slow(dst_ptr, src_ptr, size);
#else
	unsigned long unalign_mask = sizeof(unsigned long) - 1;
	unsigned long src_unalign = (unsigned long) src_ptr & unalign_mask;
	unsigned long dst_unalign = (unsigned long) dst_ptr & unalign_mask;
	if ( src_unalign != dst_unalign )
		return memcpy_slow(dst_ptr, src_ptr, size);

	union
	{
		unsigned long srcval;
		const unsigned char* restrict src8;
		const uint16_t* restrict src16;
		const uint32_t* restrict src32;
		const uint64_t* restrict src64;
		const unsigned long* restrict srcul;
	} su;
	su.srcval = (unsigned long) src_ptr;

	union
	{
		unsigned long dstval;
		unsigned char* restrict dst8;
		uint16_t* restrict dst16;
		uint32_t* restrict dst32;
		uint64_t* restrict dst64;
		unsigned long* restrict dstul;
	} du;
	du.dstval = (unsigned long) dst_ptr;

	if ( dst_unalign )
	{
		if ( 1 <= size && !(du.dstval & (1-1)) && (du.dstval & (2-1)) )
			*du.dst8++ = *su.src8++,
			size -= 1;

		if ( 2 <= size && !(du.dstval & (2-1)) && (du.dstval & (4-1)) )
			*du.dst16++ = *su.src16++,
			size -= 2;

	#if 8 <= __SIZEOF_LONG__
		if ( 4 <= size && !(du.dstval & (4-1)) && (du.dstval & (8-1)) )
			*du.dst32++ = *su.src32++,
			size -= 4;
	#endif
	}

	size_t num_copies = size / sizeof(unsigned long);
	for ( size_t i = 0; i < num_copies; i++ )
		*du.dstul++ = *su.srcul++;

	size -= num_copies * sizeof(unsigned long);

	if ( size )
	{
	#if 8 <= __SIZEOF_LONG__
		if ( 4 <= size )
			*du.dst32++ = *su.src32++,
			size -= 4;
	#endif

		if ( 2 <= size )
			*du.dst16++ = *su.src16++,
			size -= 2;

		if ( 1 <= size )
			*du.dst8++ = *su.src8++,
			size -= 1;
	}

	return dst_ptr;
#endif
}
