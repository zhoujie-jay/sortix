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

    wchar/wcsstr.c
    Locate a wide substring.

*******************************************************************************/

#include <stdbool.h>
#include <wchar.h>

// TODO: This simple and hacky implementation runs in O(N^2) even though this
// problem can be solved in O(N).
wchar_t* wcsstr(const wchar_t* restrict haystack,
                const wchar_t* restrict needle)
{
	if ( !needle[0] )
		return (wchar_t*) haystack;
	for ( size_t i = 0; haystack[i]; i++ )
	{
		bool diff = false;
		for ( size_t j = 0; needle[j]; j++ )
		{
			if ( (diff = haystack[i+j] != needle[j]) )
				break;
		}
		if ( diff )
			continue;
		return (wchar_t*) haystack + i;
	}
	return NULL;
}
