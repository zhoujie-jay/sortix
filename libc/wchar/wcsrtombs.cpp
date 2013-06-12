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

    wchar/wcsrtombs.cpp
    Convert a wide-character string to multibyte string.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern "C" size_t wcsrtombs(char* dst, const wchar_t** src_ptr, size_t dst_len,
                            mbstate_t* ps)
{
	assert(src_ptr && *src_ptr);
	// Avoid changing *src_ptr if dst is NULL.
	const wchar_t* local_src_ptr = *src_ptr;
	if ( !dst )
		src_ptr = &local_src_ptr;
	// For some reason, the standards don't mandate that the secret ps variable
	// is reset when ps is NULL, unlike mbstowcs that always resets this
	// variable. We'll avoid resetting the variable here in case any programs
	// actually take advantage of this fact.
	static mbstate_t static_ps;
	if ( !ps )
		ps = &static_ps;
	size_t ret = 0;
	size_t src_len = wcslen(*src_ptr);
	char buf[MB_CUR_MAX];
	while ( !dst || dst_len )
	{
		mbstate_t saved_ps = *ps;
		size_t produced = wcrtomb(buf, **src_ptr, ps);
		if ( produced == (size_t) -1 )
			return (size_t) -1;
		if ( dst && dst_len < produced )
		{
			*ps  = saved_ps;
			break;
		}
		memcpy(dst, buf, produced);
		if ( **src_ptr == L'\0' )
		{
			ret += produced - 1; // Don't count the '\0' byte.
			*src_ptr = NULL;
			break;
		}
		ret += produced;
		(*src_ptr)++;
		src_len--;
		if ( dst )
			dst += produced,
			dst_len -= produced;
		ret++;
	}
	return ret;
}
