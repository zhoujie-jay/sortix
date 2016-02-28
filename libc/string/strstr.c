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

    string/strstr.c
    Locate a substring.

*******************************************************************************/

#include <stdbool.h>
#include <string.h>

// TODO: This simple and hacky implementation runs in O(N^2) even though this
// problem can be solved in O(N).
char* strstr(const char* haystack, const char* needle)
{
	if ( !needle[0] )
		return (char*) haystack;
	for ( size_t i = 0; haystack[i]; i++ )
	{
		bool diff = false;
		for ( size_t j = 0; needle[j]; j++ )
		{
			if ( haystack[i+j] != needle[j] ) { diff = true; break; }
		}
		if ( diff )
			continue;
		return (char*) haystack + i;
	}
	return NULL;
}
