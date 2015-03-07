/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of Sortix libmount.

    Sortix libmount is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libmount is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libmount. If not, see <http://www.gnu.org/licenses/>.

    util.h
    Utility functions.

*******************************************************************************/

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
