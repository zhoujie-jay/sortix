/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2015.

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

    disk/ata/ata.h
    Driver for ATA.

*******************************************************************************/

#ifndef SORTIX_DISK_ATA_ATA_H
#define SORTIX_DISK_ATA_ATA_H

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {
namespace ATA {

void Init(const char* devpath, Ref<Descriptor> dev);

} // namespace ATA
} // namespace Sortix

#endif
