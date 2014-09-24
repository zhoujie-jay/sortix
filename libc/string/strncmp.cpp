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

    string/strncmp.cpp
    Compares a prefix of two strings.

*******************************************************************************/

#include <string.h>

extern "C" int strncmp(const char* a, const char* b, size_t max_count)
{
	for ( size_t i = 0; i < max_count; i++ )
	{
		unsigned char ac = (unsigned char) a[i];
		unsigned char bc = (unsigned char) b[i];
		if ( ac == '\0' && bc == '\0' )
			return 0;
		if ( ac < bc )
			return -1;
		if ( ac > bc )
			return 1;
	}
	return 0;
}
