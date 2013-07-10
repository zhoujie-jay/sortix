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

    sortix/kernel/crc32.h
    Calculates a CRC32 Checksum of binary data.

*******************************************************************************/

#ifndef SORTIX_CRC32_H
#define SORTIX_CRC32_H

namespace Sortix {
namespace CRC32 {

const uint32_t START_SEED = 0xFFFFFFFF;
void GenerateTable(uint32_t tabel[256]);
uint32_t Continue(uint32_t tabel[256], uint32_t crc, uint8_t* buf, size_t size);
uint32_t Finish(uint32_t crc);
uint32_t Hash(uint8_t* block, size_t size);

} // namespace CRC32
} // namespace Sortix

#endif
