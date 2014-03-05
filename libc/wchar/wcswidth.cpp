/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    wchar/wcswidth.cpp
    Number of column positions of a wide-character string.

*******************************************************************************/

#include <wchar.h>

extern "C" int wcswidth(const wchar_t* wcs, size_t n)
{
	int result = 0;
	for ( size_t i = 0; i < n && wcs[n]; i++ )
	{
		int columns = wcwidth(wcs[n]);
		if ( columns < 0 )
			return -1;
		result += columns;
	}
	return result;
}
