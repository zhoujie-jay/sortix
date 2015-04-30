/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    sortix/kernel/ps2.h
    Various interfaces for keyboard devices and layouts.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_PS2_H
#define INCLUDE_SORTIX_KERNEL_PS2_H

#include <stdint.h>

namespace Sortix {

class PS2Device
{
public:
	virtual ~PS2Device() { }
	virtual void PS2DeviceInitialize(void* send_ctx, bool (*send)(void*, uint8_t),
                                     uint8_t* id, size_t id_size) = 0;
	virtual void PS2DeviceOnByte(uint8_t byte) = 0;

};

} // namespace Sortix

#endif
