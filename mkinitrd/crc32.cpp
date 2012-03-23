/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.
	COPYRIGHT(C) KRZYSZTOF DABROWSKI 1999, 2000.
	COPYRIGHT(C) ElysiuM deeZine 1999, 2000.
	Based on implementation by Finn Yannick Jacobs.

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

	crc32.cpp
	Calculates a CRC32 Checksum on binary data.

*******************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include "crc32.h"

void GenerateCRC32Table(uint32_t tabel[256])
{
	uint32_t poly = 0xEDB88320U;
	for ( uint32_t i = 0; i < 256; i++ )
	{
		uint32_t crc = i;
		for ( uint32_t j = 8; 0 < j; j-- )
		{
			if ( crc & 1 ) { crc = (crc >> 1) ^ poly; }
			else { crc >>= 1; }
		}
		tabel[i] = crc;
	}
}

uint32_t ContinueCRC32(uint32_t tabel[256], uint32_t crc, uint8_t* block,
                       size_t size)
{
	for ( size_t i = 0; i < size; i++ )
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ tabel[(crc ^ *block++) & 0xFF];
	}
	return crc;
}

uint32_t FinishCRC32(uint32_t crc)
{
	return crc ^ 0xFFFFFFFF;
}

uint32_t CRC32(uint8_t* block, size_t size)
{
	uint32_t tabel[256];
	GenerateCRC32Table(tabel);
	return FinishCRC32(ContinueCRC32(tabel, CRC32_START_SEED, block, size));
}

bool CRC32File(uint32_t* result, const char* name, int fd, off_t offset,
               off_t length)
{
	uint32_t tabel[256];
	GenerateCRC32Table(tabel);
	uint32_t crc = CRC32_START_SEED;
	const size_t BUFFER_SIZE = 16UL * 1024UL;
	uint8_t buffer[BUFFER_SIZE];
	off_t sofar = 0;
	ssize_t amount;
	while ( sofar < length &&
	        0 < (amount = pread(fd, buffer, BUFFER_SIZE, offset + sofar)) )
	{
		if ( length - sofar < amount ) { amount = length - sofar; }
		crc = ContinueCRC32(tabel, crc, buffer, amount);
		sofar += amount;
	}
	if ( amount < 0 ) { error(0, errno, "read: %s", name); return false; }
	if ( sofar < length ) { error(0, EIO, "read: %s", name); return false; }
	crc = FinishCRC32(crc);
	*result = crc;
	return true;
}
