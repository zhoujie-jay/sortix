/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	stdio.c
	Sets up stdin, stdout, stderr.

******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "fdio.h"

FILE* stdin;
FILE* stdout;
FILE* stderr;

int init_stdio()
{
	stdin = fdio_newfile(0, "r");
	stdout = fdio_newfile(1, "w");
	stderr = fdio_newfile(2, "w");
	return 0;
}

int getchar(void)
{
	return fgetc(stdin);
}

int putchar(int c)
{
	return fputc(c, stdout);
}

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
		if ( (c = getc(fp)) == EOF ) { goto cleanup; }
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

ssize_t getline(char** lineptr, size_t* n, FILE* fp)
{
	return getdelim(lineptr, n, '\n', fp);
}

char* sortix_gets(void)
{
	char* buf = NULL;
	size_t n;
	if ( getline(&buf, &n, stdin) < 0 ) { return NULL; }
	size_t linelen = strlen(buf);
	if ( linelen && buf[linelen-1] == '\n' ) { buf[linelen-1] = 0; }
	return buf;
}

int puts(const char* str)
{
	return printf("%s\n", str) < 0 ? EOF : 1;
}

