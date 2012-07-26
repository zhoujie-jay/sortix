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

	strcspn.cpp
	Search a string for a set of characters.

*******************************************************************************/

#include <string.h>

extern "C" size_t strcspn(const char* str, const char* reject)
{
	size_t rejectlen = 0;
	while ( reject[rejectlen] ) { rejectlen++; }
	for ( size_t result = 0; true; result++ )
	{
		char c = str[result];
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
