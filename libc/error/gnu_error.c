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

    error/gnu_error.c
    Prints an error message to stderr and optionally exits the process.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gnu_error(int status, int errnum, const char *format, ...)
{
	flockfile(stderr);

	fprintf_unlocked(stderr, "%s: ", program_invocation_name);

	va_list list;
	va_start(list, format);
	vfprintf_unlocked(stderr, format, list);
	va_end(list);

	if ( errnum )
		fprintf_unlocked(stderr, ": %s", strerror(errnum));
	fprintf_unlocked(stderr, "\n");

	funlockfile(stderr);

	if ( status )
		exit(status);
}
