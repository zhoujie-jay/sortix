/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2006, 2010, 2011, 2012 Mark Adler
 * Adapted by Jonas 'Sortie' Termansen for Sortix libmount in 2015.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <stddef.h>
#include <unistd.h>

#include <mount/gpt.h>

uint32_t gpt_crc32(const void* buffer_ptr, size_t size)
{
	/* generate a crc for every 8-bit value */
	uint32_t poly = 0xedb88320UL;
	uint32_t crc_table[256];
	for ( unsigned int n = 0; n < 256; n++ )
	{
		uint32_t c = n;
		for ( unsigned int k = 0; k < 8; k++ )
			c = c & 1 ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}

	/* calculate bytewise crc of input buffer */
	const unsigned char* buffer = (const unsigned char*) buffer_ptr;
	uint32_t crc = 0xffffffffUL;
	for ( size_t i = 0; i < size; i++ )
		crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	return crc ^ 0xffffffffUL;
}
