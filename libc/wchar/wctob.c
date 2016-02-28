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

    wchar/wctob.c
    Wide-character to single-byte conversion.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int wctob(wint_t wc)
{
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	char mb[MB_CUR_MAX];
	int saved_errno = errno;
	size_t status = wcrtomb(mb, wc, &ps);
	if ( status == (size_t) -1 )
		return errno = saved_errno, EOF;
	if ( 2 <= status )
		return EOF;
	return (int) (unsigned char) mb[0];
}
