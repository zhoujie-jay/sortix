/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	getdelim.cpp
	Delimited string input.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* fp)
{
	if ( !lineptr || (*lineptr && !n) || !fp ) { errno = EINVAL; return -1; }
	const size_t DEFAULT_BUFSIZE = 32UL;
	int malloced = !*lineptr;
	if ( malloced ) { *lineptr = (char*) malloc(DEFAULT_BUFSIZE); }
	if ( !*lineptr ) { return -1; }
	size_t bufsize = malloced ? DEFAULT_BUFSIZE : *n;
	if ( n ) { *n = bufsize; }
	ssize_t written = 0;
	int c;
	do
	{
		if ( (c = getc(fp)) == EOF )
		{
			if ( written ) { break; } else { goto cleanup; }
		}
		if ( bufsize <= (size_t) written + 1UL )
		{
			size_t newbufsize = 2UL * bufsize;
			char* newbuf = (char*) realloc(*lineptr, newbufsize);
			if ( !newbuf ) { goto cleanup; }
			bufsize = newbufsize;
			if ( n ) { *n = bufsize; }
			*lineptr = newbuf;
		}
		(*lineptr)[written++] = c;
	} while ( c != delim );
	(*lineptr)[written] = 0;
	return written;

cleanup:
	free(malloced ? *lineptr : NULL);
	return -1;
}
