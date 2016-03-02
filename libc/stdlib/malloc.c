/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdlib/malloc.c
 * Allocates a chunk of memory from the dynamic memory heap.
 */

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

void* malloc(size_t original_size)
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

#include <sys/mman.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

void* malloc(size_t original_size)
{
	if ( !original_size )
		original_size = 1;
	size_t size = -(-original_size & ~15UL);
	size_t needed = 16 + size;
	needed = HEAP_ALIGN_PAGEUP(needed);
	size_t virtsize = needed + HEAP_PAGE_SIZE;
#ifdef __is_sortix_libk
	void* from = libk_mmap(virtsize, PROT_KREAD | PROT_KWRITE);
#else
	void* from = mmap(NULL, virtsize, PROT_READ | PROT_WRITE,
	                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
	if ( from == MAP_FAILED )
		return NULL;
	void* guard = (char*) from + needed;
#ifdef __is_sortix_libk
	libk_mprotect(guard, HEAP_PAGE_SIZE, PROT_NONE);
#else
	if ( mprotect(guard, HEAP_PAGE_SIZE, PROT_NONE) < 0 )
		return munmap(from, virtsize), (void*) NULL;
#endif
	struct heap_alloc* addralloc_ptr = (struct heap_alloc*) from;
	addralloc_ptr->from = (uintptr_t) from;
	addralloc_ptr->size = virtsize;
	size_t offset = needed - size;
	return (void*) (addralloc_ptr->from + offset);
}

#endif
