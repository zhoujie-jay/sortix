/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * addralloc.cpp
 * Class to keep track of mount points.
 */

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
