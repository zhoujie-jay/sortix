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

	fgets.cpp
	Reads a string from a FILE.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" char* fgets(char* dest, int size, FILE* fp)
{
	if ( size <= 0 ) { errno = EINVAL; return NULL; }
	int i;
	for ( i = 0; i < size-1; i++ )
	{
		int c = getc(fp);
		if ( c == EOF )
		{
			if ( ferror(fp) ) { return NULL; }
			else { i++; break; } /* EOF */
		}
		dest[i] = c;
		if ( c == '\n' ) { i++; break; }
	}

	dest[i] = '\0';
	return dest;
}

