/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    wchar/wcsrchr.cpp
    Searches a string for a specific character.

*******************************************************************************/

#include <wchar.h>

extern "C" wchar_t* wcsrchr(const wchar_t* str, wchar_t uc)
{
	const wint_t* ustr = (const wint_t*) str;
	const wchar_t* last = NULL;
	for ( size_t i = 0; true; i++ )
	{
		if ( ustr[i] == (wint_t) uc )
			last = str + i;
		if ( !ustr[i] )
			break;
	}
	return (wchar_t*) last;
}
