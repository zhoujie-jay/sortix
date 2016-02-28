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

    wchar/wcslcpy.c
    Copies a string and truncates it if the destination is too small.

*******************************************************************************/

#include <wchar.h>

size_t wcslcpy(wchar_t* restrict dest, const wchar_t* restrict src, size_t size)
{
	if ( !size )
		return wcslen(src);
	size_t result;
	for ( result = 0; result < size-1 && src[result]; result++ )
		dest[result] = src[result];
	dest[result] = L'\0';
	return result + wcslen(src + result);
}
