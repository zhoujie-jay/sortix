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

    wchar/btowc.c
    Single byte to wide character conversion.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

wint_t btowc(int ic)
{
	if ( ic == EOF )
		return WEOF;
	unsigned char uc = (unsigned char) ic;
	char c = (char) uc;
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	wchar_t wc;
	int saved_errno = errno;
	size_t status = mbrtowc(&wc, (const char*) &c, 1, &ps);
	if ( status == (size_t) -1 || status == (size_t) -2 )
		return errno = saved_errno, WEOF;
	return (wint_t) wc;
}
