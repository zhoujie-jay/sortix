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

    stdlib/realloc.cpp
    Reallocates a chunk of memory from the dynamic memory heap.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#if defined(HEAP_NO_ASSERT)
#define __heap_verify() ((void) 0)
#undef assert
#define assert(x) do { ((void) 0); } while ( 0 )
#endif

#if !defined(HEAP_GUARD_DEBUG)

extern "C" void* realloc(void* ptr, size_t requested_size)
{
	if ( !ptr )
		return malloc(requested_size);

	if ( !heap_size_has_bin(requested_size) )
		return errno = ENOMEM, (void*) NULL;

	// Decide how big an allocation we would like to make.
	size_t requested_chunk_outer_size = sizeof(struct heap_chunk) +
	                                    sizeof(struct heap_chunk_post);
	size_t requested_chunk_inner_size = heap_align(requested_size);
	size_t requested_chunk_size = requested_chunk_outer_size +
                                  requested_chunk_inner_size;

	if ( !heap_size_has_bin(requested_chunk_size) )
		return errno = ENOMEM, (void*) NULL;

	__heap_lock();
	__heap_verify();

	// Retrieve the chunk that contains this allocation.
	struct heap_chunk* chunk = heap_data_to_chunk((uint8_t*) ptr);

	assert(chunk->chunk_magic == HEAP_CHUNK_MAGIC);
	assert(heap_chunk_to_post(chunk)->chunk_magic == HEAP_CHUNK_MAGIC);
	assert(heap_chunk_to_post(chunk)->chunk_size == chunk->chunk_size);

	// Do nothing if the chunk already has the ideal size.
	if ( chunk->chunk_size == requested_chunk_size )
	{
		__heap_verify();
		__heap_unlock();
		return heap_chunk_to_data(chunk);
	}

	// If the ideal size is smaller than the current, attempt the shrink the
	// allocation if a new chunk can be created.
	if ( requested_chunk_size < chunk->chunk_size )
	{
		assert(requested_chunk_size <= chunk->chunk_size);
		if ( heap_can_split_chunk(chunk, requested_chunk_size) )
			heap_split_chunk(chunk, requested_chunk_size);
		__heap_verify();
		__heap_unlock();
		return heap_chunk_to_data(chunk);
	}

	// TODO: What if the right neighbor is the part edge?

	// If we need to expand the chunk, attempt to combine it with its right
	// neighbor if it is large enough.
	struct heap_chunk* right;
	if ( (right = heap_chunk_right(chunk)) &&
	     !heap_chunk_is_used(right) &&
	     requested_chunk_size <= chunk->chunk_size + right->chunk_size )
	{
		heap_remove_chunk(right);
		heap_chunk_format((uint8_t*) chunk, chunk->chunk_size + right->chunk_size);
		assert(requested_chunk_size <= chunk->chunk_size);
		if ( heap_can_split_chunk(chunk, requested_chunk_size) )
			heap_split_chunk(chunk, requested_chunk_size);
		__heap_verify();
		__heap_unlock();
		return heap_chunk_to_data(chunk);
	}

	// It appears that we cannot retain the orignal allocation location and we
	// will have to relocate the allocation elsewhere to expand it.

	size_t orignal_ptr_size = heap_chunk_data_size(chunk);

	__heap_verify();
	__heap_unlock();

	assert(orignal_ptr_size < requested_size);

	void* result = malloc(requested_size);
	if ( !result )
		return (void*) NULL;

	memcpy(result, ptr, orignal_ptr_size);

	free(ptr);

	return result;
}

#else

#if defined(__is_sortix_kernel)
#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/memorymanagement.h>
#endif

extern "C" void* realloc(void* ptr, size_t requested_size)
{
	if ( !ptr )
		return malloc(requested_size);

#if defined(__is_sortix_kernel)
	using namespace Sortix;
	addralloc_t* alloc_ptr =
		(addralloc_t*) Page::AlignDown((uintptr_t) ptr - 16);
	assert(alloc_ptr->from == (uintptr_t) alloc_ptr);
	addralloc_t alloc = *alloc_ptr;
	size_t size = (alloc.from + alloc.size - Page::Size()) - (uintptr_t) ptr;
#else
	struct heap_alloc* alloc_ptr =
		(struct heap_alloc*) HEAP_ALIGN_PAGEDOWN((uintptr_t) ptr - 16);
	assert(alloc_ptr->from == (uintptr_t) alloc_ptr);
	struct heap_alloc alloc = *alloc_ptr;
	size_t size = (alloc.from + alloc.size - HEAP_PAGE_SIZE) - (uintptr_t) ptr;
#endif
	if ( requested_size <= size )
		return ptr;
	void* replacement = malloc(requested_size);
	if ( !replacement )
		return NULL;
	memcpy(replacement, ptr, size);
	free(ptr);
	return replacement;
}

#endif
