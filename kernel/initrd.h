/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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
    Extracts initrds into the initial memory filesystem.

*******************************************************************************/

#ifndef SORTIX_INITRD_H
#define SORTIX_INITRD_H

#include <sortix/kernel/refcount.h>

struct multiboot_info;

namespace Sortix {

class Descriptor;

void ExtractModules(struct multiboot_info* bootinfo, Ref<Descriptor> root);

} // namespace Sortix

#endif
