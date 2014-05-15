/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    addralloc.cpp
    Class to keep track of mount points.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>

namespace Sortix {

size_t aux_allocated = 0;
size_t heap_allocated = 0;
kthread_mutex_t alloc_lock = KTHREAD_MUTEX_INITIALIZER;

// TODO: Kernel address space is allocated simply by increasing the pointer,
// with no support for freeing it. This is not that big a problem at this point,
// since address space is consumed at boot and then used forever. When it
// becomes normal for devices to come and go, just recode these functions and
// everyone should do the right thing.

bool AllocateKernelAddress(addralloc_t* ret, size_t size)
{
	size = Page::AlignUp(size);
	ScopedLock lock(&alloc_lock);
	addr_t kmem_from;
	size_t kmem_size;
	Memory::GetKernelVirtualArea(&kmem_from, &kmem_size);
	addr_t aux_reached = kmem_from + kmem_size - aux_allocated;
	size_t heap_reached = kmem_from + heap_allocated;
	size_t unused_left = aux_reached - heap_reached;
	if ( unused_left < size )
		return errno = ENOMEM, false;
	aux_allocated += size;
	ret->from = kmem_from + kmem_size - aux_allocated;
	ret->size = size;
	return true;
}

// TODO: Proper deallocation support!
void FreeKernelAddress(addralloc_t* alloc)
{
	if ( !alloc->from )
		return;

	ScopedLock lock(&alloc_lock);
	addr_t kmem_from;
	size_t kmem_size;
	Memory::GetKernelVirtualArea(&kmem_from, &kmem_size);
	addr_t aux_reached = kmem_from + kmem_size - aux_allocated;
	if ( alloc->from == aux_reached )
		aux_allocated -= alloc->size;

	alloc->from = 0;
	alloc->size = 0;
}

size_t ExpandHeap(size_t increase)
{
	increase = Page::AlignUp(increase);
	ScopedLock lock(&alloc_lock);
	addr_t kmem_from;
	size_t kmem_size;
	Memory::GetKernelVirtualArea(&kmem_from, &kmem_size);
	addr_t aux_reached = kmem_from + kmem_size - aux_allocated;
	size_t heap_reached = kmem_from + heap_allocated;
	size_t unused_left = aux_reached - heap_reached;
	if ( unused_left < increase )
		return errno = ENOMEM, 0;
	heap_allocated += increase;
	return increase;
}

void ShrinkHeap(size_t decrease)
{
	assert(decrease == Page::AlignUp(decrease));
	ScopedLock lock(&alloc_lock);
	assert(decrease <= heap_allocated);
	heap_allocated -= decrease;
}

// No need for locks in these three functions, since only the heap calls these
// and it already uses an internal lock, and heap_allocated will not change
// unless the heap calls ExpandHeap.

addr_t GetHeapLower()
{
	addr_t kmem_from;
	size_t kmem_size;
	Memory::GetKernelVirtualArea(&kmem_from, &kmem_size);
	(void) kmem_size;
	return kmem_from;
}

addr_t GetHeapUpper()
{
	addr_t kmem_from;
	size_t kmem_size;
	Memory::GetKernelVirtualArea(&kmem_from, &kmem_size);
	(void) kmem_size;
	return kmem_from + heap_allocated;
}

size_t GetHeapSize()
{
	return heap_allocated;
}

} // namespace Sortix
