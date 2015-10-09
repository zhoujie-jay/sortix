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

    stdlib/malloc.cpp
    Allocates a chunk of memory from the dynamic memory heap.

*******************************************************************************/

#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(HEAP_NO_ASSERT)
#define __heap_verify() ((void) 0)
#undef assert
#define assert(x) do { ((void) 0); } while ( 0 )
#endif

#if !defined(HEAP_GUARD_DEBUG)

extern "C" void* malloc(size_t original_size)
{
	if ( !heap_size_has_bin(original_size) )
		return errno = ENOMEM, (void*) NULL;

	// Decide how big an allocation we would like to make.
	size_t chunk_outer_size = sizeof(struct heap_chunk) +
	                          sizeof(struct heap_chunk_post);
	size_t chunk_inner_size = heap_align(original_size);
	size_t chunk_size = chunk_outer_size + chunk_inner_size;

	if ( !heap_size_has_bin(chunk_size) )
		return errno = ENOMEM, (void*) NULL;

	// Decide which bins are large enough for our allocation.
	size_t smallest_desirable_bin = heap_bin_for_allocation(chunk_size);
	size_t smallest_desirable_bin_size = heap_size_of_bin(smallest_desirable_bin);
	size_t desirable_bins = ~0UL << smallest_desirable_bin;

	__heap_lock();
	__heap_verify();

	// Determine whether there are any bins that we can use.
	size_t usable_bins = desirable_bins & __heap_state.bin_filled_bitmap;

	// If there are no usable bins, attempt to expand the current part of the
	// heap or create a new part.
	if ( !usable_bins && __heap_expand_current_part(smallest_desirable_bin_size) )
		usable_bins = desirable_bins & __heap_state.bin_filled_bitmap;

	// If we failed to expand the current part or make a new one - then we are
	// officially out of memory until someone deallocates something.
	if ( !usable_bins )
	{
		__heap_verify();
		__heap_unlock();
		return (void*) NULL;
	}

	// Pick the smallest of the usable bins.
	size_t bin_index = heap_bsf(usable_bins);

	// Pick the first element of this bins linked list. This is our allocation.
	struct heap_chunk* result_chunk = __heap_state.bin[bin_index];
	assert(result_chunk);
	assert(HEAP_IS_POINTER_ALIGNED(result_chunk, result_chunk->chunk_size));

	assert(chunk_size <= result_chunk->chunk_size);

	// Mark our chosen chunk as used and remove it from its bin.
	heap_remove_chunk(result_chunk);

	// If our chunk is larger than what we really needed and it is possible to
	// split the chunk into two, then we should split off a part of it and
	// return it to the heap for further allocation.
	if ( heap_can_split_chunk(result_chunk, chunk_size) )
		heap_split_chunk(result_chunk, chunk_size);

	__heap_verify();
	__heap_unlock();

	// Return the inner data associated with the chunk to the caller.
	return heap_chunk_to_data(result_chunk);
}

#else

#if defined(__is_sortix_kernel)
#include <sortix/mman.h>
#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/memorymanagement.h>
#else
#include <sys/mman.h>
#endif

extern "C" void* malloc(size_t original_size)
{
	if ( !original_size )
		original_size = 1;
	size_t size = -(-original_size & ~15UL);
	size_t needed = 16 + size;
#if defined(__is_sortix_kernel)
	using namespace Sortix;
	needed = Page::AlignUp(needed);
	size_t virtsize = needed + Page::Size();
	addralloc_t addralloc;
	if ( !AllocateKernelAddress(&addralloc, virtsize) )
		return NULL;
	if ( !Memory::MapRange(addralloc.from, addralloc.size - Page::Size(),
	                       PROT_KREAD | PROT_KWRITE, PAGE_USAGE_KERNEL_HEAP) )
	{
		FreeKernelAddress(&addralloc);
		return NULL;
	}
	Memory::Flush();
	addralloc_t* addralloc_ptr = (addralloc_t*) (addralloc.from);
	*addralloc_ptr = addralloc;
#else
	needed = HEAP_ALIGN_PAGEUP(needed);
	size_t virtsize = needed + HEAP_PAGE_SIZE;
	void* from = mmap(NULL, virtsize, PROT_READ | PROT_WRITE,
	                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if ( from == MAP_FAILED )
		return NULL;
	void* guard = (char*) from + needed;
	if ( mprotect(guard, HEAP_PAGE_SIZE, PROT_NONE) < 0 )
		return munmap(from, virtsize), (void*) NULL;
	struct heap_alloc* addralloc_ptr = (struct heap_alloc*) from;
	addralloc_ptr->from = (uintptr_t) from;
	addralloc_ptr->size = virtsize;
#endif
	size_t offset = needed - size;
	return (void*) (addralloc_ptr->from + offset);
}

#endif
