/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    x86-family/x86-family.h
    CPU stuff for the x86 CPU family.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_H
#define SORTIX_X86_FAMILY_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {
namespace CPU {

void OutPortB(uint16_t port, uint8_t value);
void OutPortW(uint16_t port, uint16_t value);
void OutPortL(uint16_t port, uint32_t value);
uint8_t InPortB(uint16_t port);
uint16_t InPortW(uint16_t port);
uint32_t InPortL(uint16_t port);
void Reboot();
void ShutDown();

} // namespace CPU
} // namespace Sortix

namespace Sortix {

const size_t FLAGS_CARRY        = 1 << 0; // 0x000001
const size_t FLAGS_RESERVED1    = 1 << 1; // 0x000002, read as one
const size_t FLAGS_PARITY       = 1 << 2; // 0x000004
const size_t FLAGS_RESERVED2    = 1 << 3; // 0x000008
const size_t FLAGS_AUX          = 1 << 4; // 0x000010
const size_t FLAGS_RESERVED3    = 1 << 5; // 0x000020
const size_t FLAGS_ZERO         = 1 << 6; // 0x000040
const size_t FLAGS_SIGN         = 1 << 7; // 0x000080
const size_t FLAGS_TRAP         = 1 << 8; // 0x000100
const size_t FLAGS_INTERRUPT    = 1 << 9; // 0x000200
const size_t FLAGS_DIRECTION    = 1 << 10; // 0x000400
const size_t FLAGS_OVERFLOW     = 1 << 11; // 0x000800
const size_t FLAGS_IOPRIVLEVEL  = 1 << 12) | 1 << 13;
const size_t FLAGS_NESTEDTASK   = 1 << 14; // 0x004000
const size_t FLAGS_RESERVED4    = 1 << 15; // 0x008000
const size_t FLAGS_RESUME       = 1 << 16; // 0x010000
const size_t FLAGS_VIRTUAL8086  = 1 << 17; // 0x020000
const size_t FLAGS_ALIGNCHECK   = 1 << 18; // 0x040000
const size_t FLAGS_VIRTINTR     = 1 << 19; // 0x080000
const size_t FLAGS_VIRTINTRPEND = 1 << 20; // 0x100000
const size_t FLAGS_ID           = 1 << 21; // 0x200000

} // namespace Sortix

#endif
