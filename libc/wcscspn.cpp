/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    wcscspn.cpp
    Search a string for a set of characters.

*******************************************************************************/

#include <wchar.h>

extern "C" size_t wcscspn(const wchar_t* str, const wchar_t* reject)
{
	size_t rejectlen = 0;
	while ( reject[rejectlen] ) { rejectlen++; }
	for ( size_t result = 0; true; result++ )
	{
		wchar_t c = str[result];
		if ( !c ) { return result; }
		bool matches = false;
		for ( size_t i = 0; i < rejectlen; i++ )
		{
			if ( str[result] != reject[i] ) { continue; }
			matches = true;
			break;
		}
		if ( matches ) { return result; }
	}
}
