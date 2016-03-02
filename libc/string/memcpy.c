/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * string/memcpy.c
 * Copy memory between non-overlapping regions.
 */

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
