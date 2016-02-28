/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    err/vwarn.c
    Print an error message to stderr.

*******************************************************************************/

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void vwarn(const char* fmt, va_list ap)
{
	int errnum = errno;
	flockfile(stderr);
	fprintf_unlocked(stderr, "%s: ", program_invocation_name);
	if ( fmt )
	{
		vfprintf_unlocked(stderr, fmt, ap);
		fputs_unlocked(": ", stderr);
	}
	fprintf_unlocked(stderr, "%s\n", strerror(errnum));
	funlockfile(stderr);
}
