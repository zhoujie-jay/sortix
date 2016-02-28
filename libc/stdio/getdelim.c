/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014, 2015.

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

    stdio/getdelim.c
    Delimited string input.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t DEFAULT_BUFSIZE = 32UL;

ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* fp)
{
	int err = EINVAL;
	flockfile(fp);
	if ( !lineptr || !n )
	{
	failure:
		fp->flags |= _FILE_STATUS_ERROR;
		funlockfile(fp);
		return errno = err, -1;
	}
	err = ENOMEM;
	if ( !*lineptr )
		*n = 0;
	size_t written = 0;
	int c;
	do
	{
		if ( written == (size_t) SSIZE_MAX )
			goto failure;
		if ( *n <= (size_t) written + 1UL )
		{
			size_t newbufsize = *n ? 2UL * *n : DEFAULT_BUFSIZE;
			char* newbuf = (char*) realloc(*lineptr, newbufsize);
			if ( !newbuf )
				goto failure;
			*lineptr = newbuf;
			*n = newbufsize;
		}
		if ( (c = getc_unlocked(fp)) == EOF )
		{
			if ( !written || feof_unlocked(fp) )
			{
				funlockfile(fp);
				return -1;
			}
		}
		(*lineptr)[written++] = c;
	} while ( c != delim );
	funlockfile(fp);
	(*lineptr)[written] = 0;
	return (ssize_t) written;
}
