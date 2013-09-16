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

    wchar/wmemmove.cpp
    Copy wide memory between potentially overlapping regions.

*******************************************************************************/

#include <stdint.h>
#include <wchar.h>

extern "C" wchar_t* wmemmove(wchar_t* dst, const wchar_t* src, size_t n)
{
	if ( (uintptr_t) dst < (uintptr_t) src )
		for ( size_t i = 0; i < n; i++ )
			dst[i] = src[i];
	else
		for ( size_t i = 0; i < n; i++ )
			dst[n-i] = src[n-i];
	return dst;
}
