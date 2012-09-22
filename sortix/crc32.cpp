/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.
	Copyright(C) Krzysztof Dabrowski 1999, 2000.
	Copyright(C) ElysiuM deeZine 1999, 2000.
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

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	crc32.cpp
	Calculates a CRC32 Checksum of binary data.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/crc32.h>

namespace Sortix {
namespace CRC32 {

void GenerateTable(uint32_t tabel[256])
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

uint32_t Continue(uint32_t tabel[256], uint32_t crc, uint8_t* block,
                  size_t size)
{
	for ( size_t i = 0; i < size; i++ )
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ tabel[(crc ^ *block++) & 0xFF];
	}
	return crc;
}

uint32_t Finish(uint32_t crc)
{
	return crc ^ 0xFFFFFFFF;
}

uint32_t Hash(uint8_t* block, size_t size)
{
	uint32_t tabel[256];
	GenerateTable(tabel);
	return Finish(Continue(tabel, START_SEED, block, size));
}

} // namespace CRC32
} // namespace Sortix
