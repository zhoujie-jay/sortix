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
 * malloc/__heap_expand_current_part.c
 * Attemps to extend the current part of the heap or create a new part.
 */

#include <sys/mman.h>

#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

bool __heap_expand_current_part(size_t requested_expansion)
{
	// Determine the current page size.
#ifdef __is_sortix_libk
	size_t page_size = libk_getpagesize();
#else
	size_t page_size = getpagesize();
#endif

	// Determine how much memory and how many pages are required. The worst
	// case is that we'll need to create a new part, so allocate enough
	// memory for that case.
	size_t needed_expansion =
		sizeof(struct heap_part) + requested_expansion + sizeof(struct heap_part_post);
	size_t needed_expansion_pages =
		needed_expansion / page_size + (needed_expansion % page_size ? 1 : 0);

	// Allocate at least a few pages to prevent inefficient allocations.
	size_t minimum_part_pages = 0;
	if ( __heap_state.current_part )
		minimum_part_pages = __heap_state.current_part->part_size / page_size;
	if ( 256 < minimum_part_pages )
		minimum_part_pages = 256;
	if ( minimum_part_pages < 4 )
		minimum_part_pages = 4;
	if ( needed_expansion_pages < minimum_part_pages )
		needed_expansion_pages = minimum_part_pages;

	// Calculate exactly how many bytes are added to the heap.
	size_t num_bytes = needed_expansion_pages * page_size;

#ifdef __is_sortix_libk
	void* mapping = libk_heap_expand(&num_bytes);
	if ( !mapping )
		return false;
	bool ideal_allocation = true;
#else
	// Decide where we'd like to allocation memory. Ideally, we'd like to extend
	// an existing part, but if there is none, then simply somewhere near the
	// start of the address space (so it can grow upwards) will do.
	void* suggestion = __heap_state.current_part ?
	                   heap_part_end(__heap_state.current_part) :
	                   (void*) (0x1 * page_size);

	// Attempt to allocate memory for the new part.

	void* mapping = mmap(suggestion, num_bytes, PROT_READ | PROT_WRITE,
	                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	// We are out of memory if we can't make a suitable allocation.
	if ( mapping == MAP_FAILED )
		return false;

	bool ideal_allocation = mapping == suggestion;
#endif

	// Extend the current part if we managed to do our ideal allocation.
	if ( __heap_state.current_part && ideal_allocation )
	{
		struct heap_part* part = __heap_state.current_part;
		struct heap_part_post* old_post = heap_part_to_post(part);
		struct heap_part* part_prev = old_post->part_prev;

		uint8_t* new_chunk_begin = (uint8_t*) mapping - sizeof(struct heap_part_post);
		size_t new_chunk_size = num_bytes;

		// Remove the last chunk in the part from the heap if it is unused, we
		// will then simply include its bytes in our new chunk below.
		struct heap_chunk* last_chunk;
		if ( (last_chunk = heap_part_last_chunk(part)) &&
		     !heap_chunk_is_used(last_chunk) )
		{
			heap_remove_chunk(last_chunk);
			new_chunk_begin = (uint8_t*) last_chunk;
			new_chunk_size += last_chunk->chunk_size;
		}

		// Extend the part by moving the end part structure further ahead.
		part->part_size += num_bytes;
		struct heap_part_post* post = heap_part_to_post(part);
		post->part_magic = HEAP_PART_MAGIC;
		post->part_prev = part_prev;
		post->part_size = part->part_size;
		post->unused[0] = 0;
		post->unused[1] = 0;
		post->unused[2] = 0;

		// Format a new heap chunk that contains the new space, possibly unified
		// with whatever unused space previously existed.
		assert(requested_expansion <= new_chunk_size);
		heap_insert_chunk(heap_chunk_format(new_chunk_begin, new_chunk_size));

		return true;
	}

	// Format a new part in our new allocation.
	struct heap_part* part = (struct heap_part*) mapping;
	part->unused[0] = 0;
	part->unused[1] = 0;
	part->unused[2] = 0;
	part->part_size = num_bytes;
	part->part_magic = HEAP_PART_MAGIC;
	struct heap_part_post* post = heap_part_to_post(part);
	post->part_magic = HEAP_PART_MAGIC;
	post->part_size = part->part_size;
	post->unused[0] = 0;
	post->unused[1] = 0;
	post->unused[2] = 0;

	// Insert our part into the part linked list.
	if ( (part->part_next = __heap_state.current_part) )
		heap_part_to_post(__heap_state.current_part)->part_prev = part;
	post->part_prev = NULL;

	// Insert a new chunk into the heap containing the unused space in the part.
	uint8_t* new_chunk_begin = (uint8_t*) mapping + sizeof(struct heap_part);
	size_t new_chunk_size =
		num_bytes - (sizeof(struct heap_part) + sizeof(struct heap_part_post));
	assert(requested_expansion <= new_chunk_size);
	heap_insert_chunk(heap_chunk_format(new_chunk_begin, new_chunk_size));

	__heap_state.current_part = part;

	return true;
}
