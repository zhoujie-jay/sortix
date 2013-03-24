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

    mbsrtowcs.cpp
    Convert a multibyte string to a wide-character string.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern "C" size_t mbsrtowcs(wchar_t* dst, const char** src_ptr, size_t dst_len,
                            mbstate_t* ps)
{
	assert(src_ptr && *src_ptr);
	// Avoid changing *src_ptr if dst is NULL.
	const char* local_src_ptr = *src_ptr;
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
	size_t src_len = strlen(*src_ptr);
	while ( !dst || dst_len )
	{
		mbstate_t saved_ps = *ps;
		size_t consumed = mbrtowc(dst, *src_ptr, src_len, ps);
		if ( consumed == (size_t) 0 )
		{
			*src_ptr = NULL;
			break;
		}
		if ( consumed == (size_t) -1 )
			return (size_t) -1;
		if ( consumed == (size_t) -2 )
		{
			*ps = saved_ps;
			break;
		}
		*src_ptr += consumed;
		src_len -= consumed;
		if ( dst )
			dst++,
			dst_len--;
		ret++;
	}
	return ret;
}
