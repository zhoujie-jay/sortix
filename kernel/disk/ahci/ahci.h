/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015, 2016.

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

    disk/ahci/ahci.h
    Driver for the Advanced Host Controller Interface.

*******************************************************************************/

#ifndef SORTIX_DISK_AHCI_AHCI_H
#define SORTIX_DISK_AHCI_AHCI_H

#include <endian.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {
namespace AHCI {

bool WaitClear(volatile little_uint32_t* reg,
               uint32_t bits,
               bool any,
               unsigned int msecs);
bool WaitSet(volatile little_uint32_t* reg,
             uint32_t bits,
             bool any,
             unsigned int msecs);
void Init(const char* devpath, Ref<Descriptor> dev);

} // namespace AHCI
} // namespace Sortix

#endif
