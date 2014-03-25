/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/kernel/pci-mmio.h
    Functions for handling PCI devices.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_PCI_MMIO_H
#define INCLUDE_SORTIX_KERNEL_PCI_MMIO_H

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/pci.h>

namespace Sortix {

const int MAP_PCI_BAR_WRITE_COMBINE = 1 << 0;

bool MapPCIBAR(addralloc_t* allocation, pcibar_t bar, int flags = 0);
void UnmapPCIBar(addralloc_t* allocation);

} // namespace Sortix

#endif
