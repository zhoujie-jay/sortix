/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	strncat.cpp
	Appends parts of a string onto another string.

*******************************************************************************/

#include <string.h>

extern "C" char* strncat(char* dest, const char* src, size_t n)
{
	size_t dest_len = strlen(dest);
	size_t i;
	for ( i = 0; i < n && src[i] != '\0'; i++ )
		dest[dest_len + i] = src[i];
	dest[dest_len + i] = '\0';
	return dest;
}
