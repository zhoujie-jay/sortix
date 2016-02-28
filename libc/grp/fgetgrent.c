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

    grp/fgetgrent.c
    Reads a group entry from a FILE in a thread-insecure manner.

*******************************************************************************/

#include <errno.h>
#include <grp.h>
#include <stdlib.h>

struct group* fgetgrent(FILE* fp)
{
	static struct group result_object;
	static char* buf = NULL;
	static size_t buflen = 0;
	if ( !buf )
	{
		size_t new_buflen = 64;
		if ( !(buf = (char*) malloc(new_buflen)) )
			return NULL;
		buflen = new_buflen;
	}
	struct group* result;
	int errnum;
retry:
	errnum = fgetgrent_r(fp, &result_object, buf, buflen, &result);
	if ( errnum == ERANGE )
	{
		size_t new_buflen = 2 * buflen;
		char* new_buf = (char*) realloc(buf, new_buflen);
		if ( !new_buf )
			return NULL;
		buf = new_buf;
		buflen = new_buflen;
		goto retry;
	}
	if ( errnum < 0 )
		return errno = errnum, (struct group*) NULL;
	return result;
}
