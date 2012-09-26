/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	calloc.cpp
	Allocates zeroed memory.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>

extern "C" void* calloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	void* result = malloc(total);
	if ( !result )
		return NULL;
	memset(result, 0, total);
	return result;
}
