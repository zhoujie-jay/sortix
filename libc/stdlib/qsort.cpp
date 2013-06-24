/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    stdlib/qsort.cpp
    Utility functions related to sorting and sorted data.

*******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// TODO: This is an ugly hack!
static void memswap(uint8_t* a, uint8_t* b, size_t size)
{
	uint8_t* tmp = (uint8_t*) malloc(size);
	if ( !tmp ) { abort(); }
	memcpy(tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, tmp, size);
	free(tmp);
}

// TODO: This is just a quick and dirty insertion sort. It'd be nice to have a
// good old quick sort here soon. Combined with the slow memswap above, this
// results in a very slow sorting algorithm.
extern "C" void qsort(void* base, size_t nmemb, size_t size,
                      int (* compare)(const void*, const void*))
{
	uint8_t* buf = (uint8_t*) base;
	for ( size_t i = 0; i < nmemb; i++ )
	{
		for ( size_t c = i; c; c-- )
		{
			size_t currentoff = c * size;
			uint8_t* current = buf + currentoff;
			size_t p = c-1;
			size_t prevoff = p * size;
			uint8_t* prev = buf + prevoff;
			int cmp = compare(prev, current);
			if ( cmp <= 0 ) { break; }
			memswap(prev, current, size);
		}
	}
}
