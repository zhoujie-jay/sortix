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
 * stdlib/free.c
 * Returns a chunk of memory to the dynamic memory heap.
 */

#include <malloc.h>
#include <stdlib.h>

#if defined(HEAP_NO_ASSERT)
#define __heap_verify() ((void) 0)
#undef assert
#define assert(x) do { ((void) 0); } while ( 0 )
#endif

#if !defined(HEAP_GUARD_DEBUG)

void free(void* addr)
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

#include <sys/mman.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

void free(void* addr)
{
	if ( !addr )
		return;

	struct heap_alloc* alloc_ptr =
		(struct heap_alloc*) HEAP_ALIGN_PAGEDOWN((uintptr_t) addr - 16);
#ifdef __is_sortix_libk
	libk_munmap((void*) alloc_ptr->from, alloc_ptr->size);
#else
	munmap((void*) alloc_ptr->from, alloc_ptr->size);
#endif
}

#endif
