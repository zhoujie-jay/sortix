/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    stdlib/calloc.c
    Allocates zeroed memory.

*******************************************************************************/

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void* calloc(size_t nmemb, size_t size)
{
	if ( size && nmemb && SIZE_MAX / size < nmemb )
		return errno = ENOMEM, (void*) NULL;
	size_t total = nmemb * size;
	void* result = malloc(total);
	if ( !result )
		return NULL;
	memset(result, 0, total);
	return result;
}
