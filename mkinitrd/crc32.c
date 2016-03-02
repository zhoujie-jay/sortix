/*
 * Copyright (c) 2012, 2016 Jonas 'Sortie' Termansen.
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
 * crc32.c
 * Calculates a CRC32 Checksum on binary data.
 */

// TODO: Remove this file and this feature after releasing Sortix 1.0. Change
//       the checksum algorithm in the initrd header to say none.

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>

#include "crc32.h"
#include "zcrc32.h"

bool CRC32File(uint32_t* result, const char* name, int fd, off_t offset,
               off_t length)
{
	uint32_t crc = crc32(0, NULL, 0);
	const size_t BUFFER_SIZE = 16UL * 1024UL;
	uint8_t buffer[BUFFER_SIZE];
	off_t sofar = 0;
	ssize_t amount = 0;
	while ( sofar < length &&
	        0 < (amount = pread(fd, buffer, BUFFER_SIZE, offset + sofar)) )
	{
		if ( length - sofar < amount ) { amount = length - sofar; }
		crc = crc32(crc, buffer, amount);
		sofar += amount;
	}
	if ( amount < 0 ) { error(0, errno, "read: %s", name); return false; }
	if ( sofar < length ) { error(0, EIO, "read: %s", name); return false; }
	*result = crc;
	return true;
}
