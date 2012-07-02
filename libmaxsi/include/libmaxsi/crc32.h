/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.
	COPYRIGHT(C) KRZYSZTOF DABROWSKI 1999, 2000.
	COPYRIGHT(C) ElysiuM deeZine 1999, 2000.
	Based on implementation by Finn Yannick Jacobs.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	crc32.h
	Calculates a CRC32 Checksum of binary data.

*******************************************************************************/

#ifndef LIBMAXSI_CRC32_H
#define LIBMAXSI_CRC32_H

namespace Maxsi {
namespace CRC32 {

const uint32_t START_SEED = 0xFFFFFFFF;
void GenerateTable(uint32_t tabel[256]);
uint32_t Continue(uint32_t tabel[256], uint32_t crc, uint8_t* buf, size_t size);
uint32_t Finish(uint32_t crc);
uint32_t Hash(uint8_t* block, size_t size);

} // namespace CRC32
} // namespace Maxsi

#endif
