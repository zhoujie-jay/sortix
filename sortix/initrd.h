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

    initrd.h
    Provides low-level access to a Sortix init ramdisk.

*******************************************************************************/

#ifndef SORTIX_INITRD_H
#define SORTIX_INITRD_H

#include <sortix/kernel/refcount.h>

namespace Sortix {

class Descriptor;

namespace InitRD {

bool ExtractInto(Ref<Descriptor> desc);
bool ExtractFromPhysicalInto(addr_t physaddr, size_t size, Ref<Descriptor> desc);

} // namespace InitRD

} // namespace Sortix

#endif
