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

    stdio/stdio.cpp
    Sets up stdin, stdout, stderr.

*******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fdio.h"

extern "C" { FILE* stdin; }
extern "C" { FILE* stdout; }
extern "C" { FILE* stderr; }

extern "C" int init_stdio()
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

extern "C" int getchar_unlocked(void)
{
	return fgetc_unlocked(stdin);
}

extern "C" int putchar_unlocked(int c)
{
	return fputc_unlocked(c, stdout);
}

extern "C" int getchar(void)
{
	flockfile(stdin);
	int ret = getchar_unlocked();
	funlockfile(stdin);
	return ret;
}

extern "C" int putchar(int c)
{
	flockfile(stdin);
	int ret = putchar_unlocked(c);
	funlockfile(stdin);
	return ret;
}

extern "C" int puts(const char* str)
{
	return printf("%s\n", str) < 0 ? EOF : 1;
}
