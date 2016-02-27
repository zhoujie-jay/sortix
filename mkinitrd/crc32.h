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

    crc32.h
    Calculates a CRC32 Checksum on binary data.

*******************************************************************************/

#ifndef CRC32_H
#define CRC32_H

// TODO: Remove this file and this feature after releasing Sortix 1.0. Change
//       the checksum algorithm in the initrd header to say none.

bool CRC32File(uint32_t* result, const char* name, int fd, off_t offset,
               off_t length);

#endif
