/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014, 2015.

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

    malloc.h
    Memory allocation internals.

*******************************************************************************/

#ifndef INCLUDE_MALLOC_H
#define INCLUDE_MALLOC_H

#include <sys/cdefs.h>

#if __is_sortix_libc
#include <__/wordsize.h>
#endif

#if __is_sortix_libc
#include <assert.h>
#if !defined(__cplusplus)
#include <stdalign.h>
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int heap_get_paranoia(void);
/* TODO: Operations to verify pointers and consistency check the heap. */

/* NOTE: The following declarations are heap internals and are *NOT* part of the
         API or ABI in any way. You should in no way depend on this information,
         except perhaps for debugging purposes. */
#if __is_sortix_libc

/* This macro governs how paranoid the malloc implementation is. It's not a
   constant as it may be a runtime function call deciding the level.
   Level 0 - disables all automatic consistency checks.
   Level 1 - is afraid that the caller may have corrupted the chunks passed to
             free and realloc and possibly their neighbors if chunk unification
             is considered.
   Level 2 - is afraid the heap has been damaged, so it verifies the entire heap
             before all heap operatations.
   Level 3 - is afraid that malloc itself is buggy and consistency checks the
             entire heap both before and after all heap operations. */
#if 1 /* Constant paranoia, better for code optimization. */
#define PARANOIA 1
#else /* Dynamic paranoia, better for debugging. */
#define PARANOIA_DEFAULT 1
#define PARANOIA heap_get_paranoia()
#endif

/* TODO: Proper logic here. */
#if !defined(PARANOIA_DEFAULT)
#if PARANOIA < 3
#undef assert
#define assert(x) do { ((void) 0); } while ( 0 )
#define HEAP_NO_ASSERT
#endif
#endif

#if !defined(HEAP_GUARD_DEBUG) && \
    defined(__is_sortix_kernel) && defined(HEAP_GUARD_DEBUG_KERNEL)
#define HEAP_GUARD_DEBUG
#endif

#if !defined(MALLOC_GUARD_DEBUG) && \
    __STDC_HOSTED__ && defined(HEAP_GUARD_DEBUG_USERLAND)
#define HEAP_GUARD_DEBUG
#endif

#if defined(HEAP_GUARD_DEBUG) && !defined(__is_sortix_kernel)
struct heap_alloc
{
	uintptr_t from;
	size_t size;
};
#define HEAP_PAGE_SIZE 4096UL
#define HEAP_ALIGN_PAGEDOWN(x) ((x) & ~(HEAP_PAGE_SIZE - 1))
#define HEAP_ALIGN_PAGEUP(x) (-(-(x) & ~(HEAP_PAGE_SIZE - 1)))
#endif

/* A magic value to identify a heap part edge. The value is selected such
   that it is not appropriately aligned like a real heap chunk pointer, such
   that there are no ambiguous cases. */
#if __WORDSIZE == 32
#define HEAP_PART_MAGIC 0xBEEFBEEF
#elif __WORDSIZE == 64
#define HEAP_PART_MAGIC 0xBEEFBEEFBEEFBEEF
#else
#warning "You need to implement HEAP_CHUNK_MAGIC for your native word width"
#endif

/* A magic value to detect wheter a chunk is used. The value is selected such
   that it is not appropriately aligned like a real heap chunk pointer, such
   that there are no ambiguous cases. */
#if __WORDSIZE == 32
#define HEAP_CHUNK_MAGIC 0xDEADDEAD
#elif __WORDSIZE == 64
#define HEAP_CHUNK_MAGIC 0xDEADDEADDEADDEAD
#else
#warning "You need to implement HEAP_CHUNK_MAGIC for your native word width"
#endif

/* The heap is split into a number of parts that each consists of a number of
   of chunks (used and unused). The heap normally is just a single part, but if
   the address space gets too fragmented, it may not be possible to extend the
   existing part. In each part of the heap, there exists no neighboring chunks
   that are both unused (they would be combined into a single chunk. It is
   possible to fully and unambiguously traverse the entire heap by following it
   as a linked list of objects (sometimes with implied relative locations). */

/* This structure describes the current heap allocation state. */
struct heap_state
{
	struct heap_part* current_part;
	struct heap_chunk* bin[sizeof(size_t) * 8];
	size_t bin_filled_bitmap;
};

/* This structure is at the very beginning of each heap part. The size variable
   includes the size of the surrounding structures. The first chunk or the end
   of the opart follows immediately (use the magic value to determine which). */
struct heap_part
{
	size_t unused[3]; /* Alignment. */
	struct heap_part* part_next;
	size_t part_magic;
	size_t part_size;
};

static_assert(sizeof(struct heap_part) * 8 == 6 * __WORDSIZE,
             "sizeof(struct heap_part) * 8 == 6 * __WORDSIZE");
static_assert(alignof(struct heap_part) * 8 == __WORDSIZE,
             "alignof(struct heap_part) * 8 == __WORDSIZE");

/* This structure immediately preceeds all heap allocations. The size variable
   includes the size of the surrounding structures. */
struct heap_chunk
{
	size_t chunk_size;
	union
	{
		size_t chunk_magic;
		struct heap_chunk* chunk_next;
	};
};

static_assert(sizeof(struct heap_chunk) * 8 == 2 * __WORDSIZE,
             "sizeof(struct heap_chunk) * 8 == 2 * __WORDSIZE");
static_assert(alignof(struct heap_chunk) * 8 == __WORDSIZE,
             "alignof(struct heap_chunk) * 8 == __WORDSIZE");

/* Inbetween these structures come the heap allocation. */

/* This structure immediately follows all heap allocations. The size variable
   includes the size of the surrounding structures. */
struct heap_chunk_post
{
	union
	{
		size_t chunk_magic;
		struct heap_chunk* chunk_prev;
	};
	size_t chunk_size;
};

static_assert(sizeof(struct heap_chunk_post) * 8 == 2 * __WORDSIZE,
             "sizeof(struct heap_chunk_post) * 8 == 2 * __WORDSIZE");
static_assert(alignof(struct heap_chunk_post) * 8 == __WORDSIZE,
             "alignof(struct heap_chunk_post) * 8 == __WORDSIZE");

/* This structure is at the very end of each heap part. The size variable
   includes the size of the surrounding structures. The first chunk in the heap
   part follows immediately. */
struct heap_part_post
{
	size_t part_size;
	size_t part_magic;
	struct heap_part* part_prev;
	size_t unused[3]; /* Alignment. */
};


static_assert(sizeof(struct heap_part_post) * 8 == 6 * __WORDSIZE,
             "sizeof(struct heap_part_post) * 8 == 6 * __WORDSIZE");
static_assert(alignof(struct heap_part_post) * 8 == __WORDSIZE,
             "alignof(struct heap_part_post) * 8 == __WORDSIZE");

/* Global secret variables used internally by the heap. */
extern struct heap_state __heap_state;

/* Internal heap functions. */
bool __heap_expand_current_part(size_t);
void __heap_lock(void);
void __heap_unlock(void);
#if !defined(HEAP_NO_ASSERT)
void __heap_verify(void);
#endif

#if !defined(HEAP_NO_ASSERT)
/* Utility function to verify addresses are well-aligned. */
__attribute__((unused)) static inline
bool heap_is_pointer_aligned(void* addr, size_t size, size_t alignment)
{
	/* It is assumed that alignment is a power of two. */
	if ( ((uintptr_t) addr) & (alignment - 1) )
		return false;
	if ( ((uintptr_t) size) & (alignment - 1) )
		return false;
	return true;
}

#define HEAP_IS_POINTER_ALIGNED(object, size) \
        heap_is_pointer_aligned((object), (size), alignof(*(object)))

/* Utility function to trap bad memory accesses. */
__attribute__((unused)) static inline
void heap_test_usable_memory(void* addr, size_t size)
{
	for ( size_t i = 0; i < size; i++ )
		(void) ((volatile unsigned char*) addr)[i];
}
#endif

/* Utility functions for accessing the most significant bit set. */
__attribute__((unused)) static inline
size_t heap_bsr(size_t value)
{
	/* TODO: There currently is no builtin bsr function. */
	return (sizeof(size_t) * 8 - 1) - __builtin_clzl(value);
}

/* Utility functions for accessing the least significant bit set. */
__attribute__((unused)) static inline
size_t heap_bsf(size_t value)
{
	return __builtin_ctzl(value);
}

/* Utility function for getting the minimum size of entries in a bin. */
__attribute__((unused)) static inline
size_t heap_size_of_bin(size_t bin)
{
	assert(bin < sizeof(size_t) * 8);
	return (size_t) 1 << bin;
}

/* Utility function for determining whether a size even has a bin. */
__attribute__((unused)) static inline
bool heap_size_has_bin(size_t size)
{
	return size <= heap_size_of_bin(sizeof(size_t) * 8 - 1);
}

/* Utility function for determining which bin a particular size belongs to. */
__attribute__((unused)) static inline
size_t heap_chunk_size_to_bin(size_t size)
{
	assert(size != 0);
	assert(heap_size_has_bin(size));

	for ( size_t i = 8 * sizeof(size_t) - 1; true; i-- )
		if ( heap_size_of_bin(i) <= size )
			return i;

	assert(false);
	__builtin_unreachable();
}

/* Utility function for determining the smallest bin from which a allocation can
   be satisfied. This is not the same as heap_chunk_size_to_bin() as it rounds
   downwards while this rounds upwards. */
__attribute__((unused)) static inline
size_t heap_bin_for_allocation(size_t size)
{
	assert(heap_size_has_bin(size));

	/* TODO: Use the bsr or bsf instructions here whatever it was! */

	for ( size_t i = 0; i < 8 * sizeof(size_t); i++ )
		if ( size <= (size_t) 1 << i )
			return i;

	assert(false);
	__builtin_unreachable();
}

/* Rounds an allocation size up to a multiple of what malloc offers. */
__attribute__((unused)) static inline size_t heap_align(size_t value)
{
	static_assert(sizeof(struct heap_chunk) + sizeof(struct heap_chunk_post) == 4 * __WORDSIZE / 8,
	             "sizeof(struct heap_chunk) + sizeof(struct heap_chunk_post) == 4 * __WORDSIZE / 8");

	size_t mask = 4 * sizeof(size_t) - 1;
	return -(-value & ~mask);
}

/* Returns the trailing structure following the given part. */
__attribute__((unused)) static inline
struct heap_part_post* heap_part_to_post(struct heap_part* part)
{
	assert(HEAP_IS_POINTER_ALIGNED(part, part->part_size));

	return (struct heap_part_post*)
		((uintptr_t) part + part->part_size - sizeof(struct heap_part_post));
}

/* Returns the part associated with the part trailing structure. */
__attribute__((unused)) static inline
struct heap_part* heap_post_to_part(struct heap_part_post* post_part)
{
	assert(HEAP_IS_POINTER_ALIGNED(post_part, post_part->part_size));

	return (struct heap_part*)
		((uintptr_t) post_part - post_part->part_size + sizeof(struct heap_part));
}

/* Returns the ending address of a heap part. */
__attribute__((unused)) static inline
void* heap_part_end(struct heap_part* part)
{
	assert(HEAP_IS_POINTER_ALIGNED(part, part->part_size));

	return (uint8_t*) part + part->part_size;
}

/* Returns whether this chunk is currently in use. */
__attribute__((unused)) static inline
bool heap_chunk_is_used(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	return chunk->chunk_magic == HEAP_CHUNK_MAGIC;
}

/* Returns the trailing structure following the given chunk. */
__attribute__((unused)) static inline
struct heap_chunk_post* heap_chunk_to_post(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	return (struct heap_chunk_post*)
		((uintptr_t) chunk + chunk->chunk_size - sizeof(struct heap_chunk_post));
}

/* Returns the chunk associated with the chunk trailing structure. */
__attribute__((unused)) static inline
struct heap_chunk* heap_post_to_chunk(struct heap_chunk_post* post_chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(post_chunk, post_chunk->chunk_size));

	return (struct heap_chunk*)
		((uintptr_t) post_chunk - post_chunk->chunk_size + sizeof(struct heap_chunk));
}

/* Returns the data inside the chunk. */
__attribute__((unused)) static inline
uint8_t* heap_chunk_to_data(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	return (uint8_t*) chunk + sizeof(struct heap_chunk);
}

/* Returns the data inside the chunk. */
__attribute__((unused)) static inline
size_t heap_chunk_data_size(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	return chunk->chunk_size - (sizeof(struct heap_chunk) + sizeof(struct heap_chunk_post));
}

/* Returns the chunk whose data starts at the given address. */
__attribute__((unused)) static inline
struct heap_chunk* heap_data_to_chunk(uint8_t* data)
{
	struct heap_chunk* chunk =
		(struct heap_chunk*) (data - sizeof(struct heap_chunk));

	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	return chunk;
}

/* Returns the chunk to the left of this one or NULL if none. */
__attribute__((unused)) static inline
struct heap_chunk* heap_chunk_left(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	struct heap_chunk_post* left_post = (struct heap_chunk_post*)
		((uintptr_t) chunk - sizeof(struct heap_chunk_post));
	assert(HEAP_IS_POINTER_ALIGNED(left_post, 0));

	if ( left_post->chunk_magic == HEAP_PART_MAGIC )
		return NULL;
	return heap_post_to_chunk(left_post);
}

/* Returns the chunk to the right of this one or NULL if none. */
__attribute__((unused)) static inline
struct heap_chunk* heap_chunk_right(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	struct heap_chunk* right = (struct heap_chunk*)
		((uintptr_t) chunk + chunk->chunk_size);
	assert(HEAP_IS_POINTER_ALIGNED(right, 0));

	if ( right->chunk_magic == HEAP_PART_MAGIC )
		return NULL;
	return right;
}

/* Formats an used chunk at the given location. */
__attribute__((unused)) static inline
struct heap_chunk* heap_chunk_format(uint8_t* addr, size_t size)
{
	assert(heap_is_pointer_aligned(addr, size, alignof(struct heap_chunk)));
#if !defined(HEAP_NO_ASSERT)
	heap_test_usable_memory(addr, size);
#endif

	struct heap_chunk* chunk = (struct heap_chunk*) addr;
	chunk->chunk_size = size;
	chunk->chunk_magic = HEAP_CHUNK_MAGIC;
	struct heap_chunk_post* post = heap_chunk_to_post(chunk);
	post->chunk_size = size;
	post->chunk_magic = HEAP_CHUNK_MAGIC;
	return chunk;
}

/* Returns the first chunk in a part. */
__attribute__((unused)) static inline
struct heap_chunk* heap_part_first_chunk(struct heap_part* part)
{
	assert(HEAP_IS_POINTER_ALIGNED(part, part->part_size));

	struct heap_chunk* chunk =
		(struct heap_chunk*) ((uintptr_t) part + sizeof(struct heap_part));
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));

	if ( chunk->chunk_magic == HEAP_PART_MAGIC )
		return NULL;
	return chunk;
}

/* Returns the last chunk in a part. */
__attribute__((unused)) static inline
struct heap_chunk* heap_part_last_chunk(struct heap_part* part)
{
	assert(HEAP_IS_POINTER_ALIGNED(part, part->part_size));

	struct heap_part_post* post = heap_part_to_post(part);
	assert(HEAP_IS_POINTER_ALIGNED(post, post->part_size));

	struct heap_chunk_post* chunk_post =
		(struct heap_chunk_post*) ((uintptr_t) post - sizeof(struct heap_chunk_post));
	assert(HEAP_IS_POINTER_ALIGNED(chunk_post, chunk_post->chunk_size));

	if ( chunk_post->chunk_magic == HEAP_PART_MAGIC )
		return NULL;
	return heap_post_to_chunk(chunk_post);
}

/* Inserts a chunk into the heap and marks it as unused. */
__attribute__((unused)) static inline
void heap_insert_chunk(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));
	struct heap_chunk_post* chunk_post = heap_chunk_to_post(chunk);
	assert(HEAP_IS_POINTER_ALIGNED(chunk_post, chunk_post->chunk_size));
	assert(chunk->chunk_size == chunk_post->chunk_size);

	/* Decide which bin this chunk belongs to. */
	assert(heap_size_has_bin(chunk->chunk_size));
	size_t chunk_bin = heap_chunk_size_to_bin(chunk->chunk_size);

	/* Insert the chunk into this bin, destroying its magic values as used. */
	if ( __heap_state.bin[chunk_bin] )
		heap_chunk_to_post(__heap_state.bin[chunk_bin])->chunk_prev = chunk;
	chunk->chunk_next = __heap_state.bin[chunk_bin];
	chunk_post->chunk_prev = NULL;
	__heap_state.bin[chunk_bin] = chunk;
	__heap_state.bin_filled_bitmap |= heap_size_of_bin(chunk_bin);
}

/* Removes a chunk from the heap and marks it as used. */
__attribute__((unused)) static inline
void heap_remove_chunk(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));
	assert(chunk->chunk_magic != HEAP_CHUNK_MAGIC);
	assert(chunk->chunk_magic != HEAP_PART_MAGIC);
	struct heap_chunk_post* chunk_post = heap_chunk_to_post(chunk);
	assert(HEAP_IS_POINTER_ALIGNED(chunk_post, chunk_post->chunk_size));
	assert(chunk->chunk_size == chunk_post->chunk_size);
	assert(chunk_post->chunk_magic != HEAP_CHUNK_MAGIC);
	assert(chunk_post->chunk_magic != HEAP_PART_MAGIC);

	/* Decide which bin this chunk belongs to. */
	assert(heap_size_has_bin(chunk->chunk_size));
	size_t chunk_bin = heap_chunk_size_to_bin(chunk->chunk_size);
	assert(__heap_state.bin_filled_bitmap & heap_size_of_bin(chunk_bin));

	/* Unlink the chunk from its current bin. */
	if ( chunk == __heap_state.bin[chunk_bin] )
	{
		assert(!chunk_post->chunk_prev);
		if ( !(__heap_state.bin[chunk_bin] = chunk->chunk_next) )
			__heap_state.bin_filled_bitmap ^= heap_size_of_bin(chunk_bin);
	}
	else
	{
		assert(chunk_post->chunk_prev);
		heap_chunk_to_post(chunk)->chunk_prev->chunk_next = chunk->chunk_next;
	}
	if ( chunk->chunk_next )
		heap_chunk_to_post(chunk->chunk_next)->chunk_prev = chunk_post->chunk_prev;

	/* Mark the chunk as used. */
	chunk->chunk_magic = HEAP_CHUNK_MAGIC;
	chunk_post->chunk_magic = HEAP_CHUNK_MAGIC;
}

/* Decides whether the chunk can be split into two. */
__attribute__((unused)) static inline
bool heap_can_split_chunk(struct heap_chunk* chunk, size_t needed)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));
	assert(needed <= chunk->chunk_size);

	size_t minimum_chunk_size = sizeof(struct heap_chunk) +
	                            sizeof(struct heap_chunk_post);
	return minimum_chunk_size <= chunk->chunk_size - needed;
}

/* Splits a chunk into two - assumes it can be split. */
__attribute__((unused)) static inline
void heap_split_chunk(struct heap_chunk* chunk, size_t needed)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));
#if !defined(HEAP_NO_ASSERT)
	heap_test_usable_memory(chunk, chunk->chunk_size);
#endif
	assert(heap_can_split_chunk(chunk, needed));

	bool chunk_is_used = heap_chunk_is_used(chunk);
	if ( !chunk_is_used )
		heap_remove_chunk(chunk);
	size_t surplus_amount = chunk->chunk_size - needed;
	heap_chunk_format((uint8_t*) chunk, needed);
	struct heap_chunk* surplus =
		heap_chunk_format((uint8_t*) chunk + needed, surplus_amount);
	if ( !chunk_is_used )
		heap_insert_chunk(chunk);
	heap_insert_chunk(surplus);
}

/* Combine a chunk with its neighbors if they are all unused. */
__attribute__((unused)) static inline
struct heap_chunk* heap_chunk_combine_neighbors(struct heap_chunk* chunk)
{
	assert(HEAP_IS_POINTER_ALIGNED(chunk, chunk->chunk_size));
#if !defined(HEAP_NO_ASSERT)
	heap_test_usable_memory(chunk, chunk->chunk_size);
#endif

	struct heap_chunk* left_chunk = heap_chunk_left(chunk);
	struct heap_chunk* right_chunk = heap_chunk_right(chunk);
	if ( right_chunk )
	{
#if !defined(HEAP_NO_ASSERT)
		heap_test_usable_memory(right_chunk, right_chunk->chunk_size);
#endif
		assert(HEAP_IS_POINTER_ALIGNED(right_chunk, right_chunk->chunk_size));
	}

	/* Attempt to combine with both the left and right chunk. */
	if ( (left_chunk && !heap_chunk_is_used(left_chunk)) &&
	     (right_chunk && !heap_chunk_is_used(right_chunk)) )
	{
		assert(HEAP_IS_POINTER_ALIGNED(left_chunk, left_chunk->chunk_size));
		assert(HEAP_IS_POINTER_ALIGNED(right_chunk, right_chunk->chunk_size));
#if !defined(HEAP_NO_ASSERT)
		heap_test_usable_memory(left_chunk, left_chunk->chunk_size);
		heap_test_usable_memory(right_chunk, right_chunk->chunk_size);
#endif

		heap_remove_chunk(chunk);
		heap_remove_chunk(left_chunk);
		heap_remove_chunk(right_chunk);
		size_t result_size = left_chunk->chunk_size + chunk->chunk_size + right_chunk->chunk_size;
		struct heap_chunk* result = heap_chunk_format((uint8_t*) left_chunk, result_size);
		heap_insert_chunk(result);
		return result;
	}

	/* Attempt to combine with the left chunk. */
	if ( left_chunk && !heap_chunk_is_used(left_chunk) )
	{
		assert(HEAP_IS_POINTER_ALIGNED(left_chunk, left_chunk->chunk_size));
#if !defined(HEAP_NO_ASSERT)
		heap_test_usable_memory(left_chunk, left_chunk->chunk_size);
#endif

		heap_remove_chunk(chunk);
		heap_remove_chunk(left_chunk);
		size_t result_size = left_chunk->chunk_size + chunk->chunk_size;
		struct heap_chunk* result = heap_chunk_format((uint8_t*) left_chunk, result_size);
		heap_insert_chunk(result);
		return result;
	}

	/* Attempt to combine with the right chunk. */
	if ( right_chunk && !heap_chunk_is_used(right_chunk) )
	{
		assert(HEAP_IS_POINTER_ALIGNED(right_chunk, right_chunk->chunk_size));
#if !defined(HEAP_NO_ASSERT)
		heap_test_usable_memory(right_chunk, right_chunk->chunk_size);
#endif

		heap_remove_chunk(chunk);
		heap_remove_chunk(right_chunk);
		size_t result_size = chunk->chunk_size + right_chunk->chunk_size;
		struct heap_chunk* result = heap_chunk_format((uint8_t*) chunk, result_size);
		heap_insert_chunk(result);
		return result;
	}

	return chunk;
}

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#if __is_sortix_libc
#include <assert.h>
#endif

#endif
