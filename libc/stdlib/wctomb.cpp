/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    stdlib/wctomb.cpp
    Convert a wide character to a multibyte sequence.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// TODO: This function is unpure and should be removed.
extern "C" int wctomb(char* s, wchar_t wc)
{
	static mbstate_t ps;
	size_t result = wcrtomb(s, wc, &ps);
	if ( !s )
		memset(&ps, 0, sizeof(ps));
	if ( result == (size_t) -1 )
		return -1;
	if ( result == (size_t) -2 )
		return -1;
	return (int) result;
}
