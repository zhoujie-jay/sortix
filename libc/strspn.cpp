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

    strspn.cpp
    Search a string for a set of characters.

*******************************************************************************/

#include <string.h>

extern "C" size_t strspn(const char* str, const char* accept)
{
	size_t acceptlen = 0;
	while ( accept[acceptlen] ) { acceptlen++; }
	for ( size_t result = 0; true; result++ )
	{
		char c = str[result];
		if ( !c ) { return result; }
		bool matches = false;
		for ( size_t i = 0; i < acceptlen; i++ )
		{
			if ( str[result] != accept[i] ) { continue; }
			matches = true;
			break;
		}
		if ( !matches ) { return result; }
	}
}
