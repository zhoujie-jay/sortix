/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    string/strlcpy.cpp
    Copies a string and truncates it if the destination is too small.

*******************************************************************************/

#include <string.h>

extern "C"
size_t strlcpy(char* restrict dest, const char* restrict src, size_t size)
{
	if ( !size )
		return strlen(src);
	size_t result;
	for ( result = 0; result < size-1 && src[result]; result++ )
		dest[result] = src[result];
	dest[result] = '\0';
	return result + strlen(src + result);
}
