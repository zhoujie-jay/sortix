/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    stdio.c
    Sets up stdin, stdout, stderr.

*******************************************************************************/

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
	// TODO: These calls require memory allocation and can fail - which we don't
	//       currently handle. How about declaring these as global objects and
	//       using fdio_install_fd instead?
	stdin = fdio_new_fd(0, "r");
	stdout = fdio_new_fd(1, "w");
	stderr = fdio_new_fd(2, "w");
	setvbuf(stderr, NULL, _IONBF, 0);
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
