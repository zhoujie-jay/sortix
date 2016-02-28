/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2016.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with Sortix. If not, see <http://www.gnu.org/licenses/>.

    crc32.c
    Calculates a CRC32 Checksum on binary data.

*******************************************************************************/

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
