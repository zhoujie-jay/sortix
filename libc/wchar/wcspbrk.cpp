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

    wchar/wcspbrk.cpp
    Search a wide string for any of a set of wide characters.

*******************************************************************************/

#include <wchar.h>

extern "C" wchar_t* wcspbrk(const wchar_t* str, const wchar_t* accept)
{
	size_t rejectlen = wcscspn(str, accept);
	if ( !str[rejectlen] )
		return NULL;
	return (wchar_t*) str + rejectlen;
}
