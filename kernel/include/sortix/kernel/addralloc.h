/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/addralloc.h
    Class to keep track of mount points.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_ADDRALLOC_H
#define INCLUDE_SORTIX_KERNEL_ADDRALLOC_H

#include <stddef.h>

#include <sortix/kernel/decl.h>

namespace Sortix {

struct addralloc_t
{
	addr_t from;
	size_t size;
};

bool AllocateKernelAddress(addralloc_t* ret, size_t size);
void FreeKernelAddress(addralloc_t* alloc);
size_t ExpandHeap(size_t increase);
void ShrinkHeap(size_t decrease);
addr_t GetHeapLower();
addr_t GetHeapUpper();
size_t GetHeapSize();

} // namespace Sortix

#endif
