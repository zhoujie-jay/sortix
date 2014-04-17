/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    wchar/wcsrtombs.cpp
    Convert a wide-character string to multibyte string.

*******************************************************************************/

#include <stdint.h>
#include <wchar.h>

extern "C"
size_t wcsrtombs(char* restrict dst,
                 const wchar_t** restrict src_ptr,
                 size_t dst_len,
                 mbstate_t* ps)
{
	return wcsnrtombs(dst, src_ptr, SIZE_MAX, dst_len, ps);
}
