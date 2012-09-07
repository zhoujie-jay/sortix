/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	bsearch.cpp
	Binary search.

*******************************************************************************/

#include <stdint.h>
#include <stdlib.h>

extern "C" void* bsearch(const void* key, const void* base, size_t nmemb,
                         size_t size, int (*compare)(const void*, const void*))
{
	const uint8_t* baseptr = (const uint8_t*) base;
	// TODO: Just a quick and surprisingly correct yet slow implementation.
	for ( size_t i = 0; i < nmemb; i++ )
	{
		const void* candidate = baseptr + i * size;
		if ( !compare(key, candidate) )
			return (void*) candidate;
	}
	return NULL;
}
