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
 * string/memset.c
 * Initializes a region of memory to a byte value.
 */

#include <stdint.h>
#include <string.h>

#include <__/wordsize.h>

void* memset(void* dest_ptr, int value, size_t length)
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
