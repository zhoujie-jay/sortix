/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    stdlib/free.cpp
    Returns a chunk of memory to the dynamic memory heap.

*******************************************************************************/

#include <malloc.h>
#include <stdlib.h>

#if defined(HEAP_NO_ASSERT)
#define __heap_verify() ((void) 0)
#undef assert
#define assert(x) do { ((void) 0); } while ( 0 )
#endif

#if !defined(HEAP_GUARD_DEBUG)

extern "C" void free(void* addr)
{
	if ( !addr )
		return;

	__heap_lock();
	__heap_verify();

	// Retrieve the chunk that contains this allocation.
	struct heap_chunk* chunk = heap_data_to_chunk((uint8_t*) addr);

	// Return the chunk to the heap.
	heap_insert_chunk(chunk);

	// Combine the chunk with its left and right neighbors if they are unused.
	heap_chunk_combine_neighbors(chunk);

	__heap_verify();
	__heap_unlock();
}

#else

#if defined(__is_sortix_kernel)
#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/memorymanagement.h>
#else
#include <sys/mman.h>
#endif

extern "C" void free(void* addr)
{
	if ( !addr )
		return;

#if defined(__is_sortix_kernel)
	using namespace Sortix;
	addralloc_t* alloc_ptr = (addralloc_t*) Page::AlignDown((uintptr_t) addr - 16);
	assert(alloc_ptr->from == (uintptr_t) alloc_ptr);
	addralloc_t alloc = *alloc_ptr;
	Memory::UnmapRange(alloc.from, alloc.size - Page::Size(), PAGE_USAGE_KERNEL_HEAP);
	FreeKernelAddress(&alloc);
#else
	struct heap_alloc* alloc_ptr =
		(struct heap_alloc*) HEAP_ALIGN_PAGEDOWN((uintptr_t) addr - 16);
	munmap((void*) alloc_ptr->from, alloc_ptr->size);
#endif
}

#endif
