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

    You should have received a copy of the GNU General Public License along
    with Sortix. If not, see <http://www.gnu.org/licenses/>.

    crc32.h
    Calculates a CRC32 Checksum on binary data.

*******************************************************************************/

#ifndef CRC32_H
#define CRC32_H

const uint32_t CRC32_START_SEED = 0xFFFFFFFF;
void GenerateCRC32Table(uint32_t tabel[256]);
uint32_t ContinueCRC32(uint32_t tabel[256], uint32_t crc, uint8_t* block,
                       size_t size);
uint32_t FinishCRC32(uint32_t crc);
uint32_t CRC32(uint8_t* block, size_t size);
bool CRC32File(uint32_t* result, const char* name, int fd, off_t offset,
               off_t length);

#endif
