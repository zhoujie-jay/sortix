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

    wchar/wcsncasecmp.cpp
    Compares a prefix of two strings ignoring case.

*******************************************************************************/

#include <wchar.h>
#include <wctype.h>

extern "C"
int wcsncasecmp(const wchar_t* a, const wchar_t* b, size_t maxcount)
{
	while ( maxcount-- )
	{
		wchar_t ac = towlower(*a++), bc = towlower(*b++);
		if ( ac == L'\0' && bc == L'\0' )
			return 0;
		if ( ac < bc )
			return -1;
		if ( ac > bc )
			return 1;
	}
	return 0;
}