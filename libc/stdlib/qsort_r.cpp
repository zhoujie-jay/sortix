/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    stdlib/qsort_r.cpp
    Sort an array.

*******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void memswap(uint8_t* a, uint8_t* b, size_t size)
{
	uint8_t tmp;
	for ( size_t i = 0; i < size; i++ )
	{
		tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}

// TODO: This is just a quick and dirty insertion sort. It'd be nice to have a
//       good old quick sort here soon.
extern "C"
void qsort_r(void* base_ptr,
             size_t num_elements,
             size_t element_size,
             int (*compare)(const void*, const void*, void*),
             void* arg)
{
	uint8_t* base = (uint8_t*) base_ptr;
	for ( size_t i = 0; i < num_elements; i++ )
	{
		for ( size_t c = i; c; c-- )
		{
			size_t currentoff = c * element_size;
			uint8_t* current = base + currentoff;
			size_t p = c-1;
			size_t prevoff = p * element_size;
			uint8_t* prev = base + prevoff;
			int cmp = compare(prev, current, arg);
			if ( cmp <= 0 ) { break; }
			memswap(prev, current, element_size);
		}
	}
}
