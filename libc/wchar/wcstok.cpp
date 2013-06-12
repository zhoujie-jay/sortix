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

    wchar/wcstok.cpp
    Extract tokens from strings.

*******************************************************************************/

#include <wchar.h>

extern "C" wchar_t* wcstok(wchar_t* str, const wchar_t* delim, wchar_t** saveptr)
{
	if ( !str && !*saveptr )
		return NULL;
	if ( !str )
		str = *saveptr;
	str += wcsspn(str, delim); // Skip leading
	if ( !*str )
		return *saveptr = NULL;
	size_t amount = wcscspn(str, delim);
	if ( str[amount] )
		*saveptr = str + amount + 1;
	else
		*saveptr = NULL;
	str[amount] = L'\0';
	return str;
}
