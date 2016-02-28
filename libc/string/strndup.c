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

    string/strndup.c
    Creates a copy of a string.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>

char* strndup(const char* input, size_t n)
{
	size_t input_size = strnlen(input, n);
	char* result = (char*) malloc(input_size + 1);
	if ( !result )
		return NULL;
	memcpy(result, input, input_size);
	result[input_size] = 0;
	return result;
}
