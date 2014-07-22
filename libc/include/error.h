/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    error.h
    Error reporting functions.

*******************************************************************************/

#ifndef INCLUDE_ERROR_H
#define INCLUDE_ERROR_H

#include <sys/cdefs.h>

__BEGIN_DECLS

void gnu_error(int status, int errnum, const char* format, ...)
	__attribute__((__format__(__printf__, 3, 4)));
#if __SORTIX_STDLIB_REDIRECTS
void error(int status, int errnum, const char* format, ...) __asm__ ("gnu_error")
	__attribute__((__format__(__printf__, 3, 4)));
#endif

__END_DECLS

#endif
