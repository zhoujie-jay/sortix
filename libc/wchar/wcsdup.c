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

    wchar/wcsdup.c
    Creates a copy of a string.

*******************************************************************************/

#include <stdlib.h>
#include <wchar.h>

wchar_t* wcsdup(const wchar_t* input)
{
	size_t input_length = wcslen(input);
	wchar_t* result = (wchar_t*) malloc(sizeof(wchar_t) * (input_length + 1));
	if ( !result )
		return NULL;
	wmemcpy(result, input, input_length + 1);
	return result;
}
