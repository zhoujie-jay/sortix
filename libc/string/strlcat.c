/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    string/strlcat.c
    Appends a string onto another string truncating if the string is too small.

*******************************************************************************/

#include <string.h>

size_t strlcat(char* restrict dest, const char* restrict src, size_t size)
{
	size_t dest_len = strnlen(dest, size);
	if ( size <= dest_len )
		return dest_len + strlen(src);
	return dest_len + strlcpy(dest + dest_len, src, size - dest_len);
}
