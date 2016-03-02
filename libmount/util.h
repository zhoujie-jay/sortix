/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * util.h
 * Utility functions.
 */

#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>

#define MALLOC_TYPE(type) (type*) malloc(sizeof(type))
#define CALLOC_TYPE(type) (type*) calloc(1, sizeof(type))

__attribute__((unused))
static bool memiszero(const void* mem, size_t size)
{
	const unsigned char* buf = (const unsigned char*) mem;
	for ( size_t i = 0; i < size; i++ )
		if ( buf[i] )
			return false;
	return true;
}

__attribute__((unused))
static bool array_add(void*** array_ptr,
                      size_t* used_ptr,
                      size_t* length_ptr,
                      void* value)
{
	void** array;
	memcpy(&array, array_ptr, sizeof(array)); // Strict aliasing.

	if ( *used_ptr == *length_ptr )
	{
		// TODO: Avoid overflow.
		size_t new_length = 2 * *length_ptr;
		if ( !new_length )
			new_length = 16;
		// TODO: Avoid overflow and use reallocarray.
		size_t new_size = new_length * sizeof(void*);
		void** new_array = (void**) realloc(array, new_size);
		if ( !new_array )
			return false;
		array = new_array;
		memcpy(array_ptr, &array, sizeof(array)); // Strict aliasing.
		*length_ptr = new_length;
	}

	memcpy(array + (*used_ptr)++, &value, sizeof(value)); // Strict aliasing.

	return true;
}

#endif
