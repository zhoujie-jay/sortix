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

    err.h
    Error reporting functions.

*******************************************************************************/

#ifndef INCLUDE_ERR_H
#define INCLUDE_ERR_H

#include <sys/cdefs.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__, __format__(__printf__, 2, 3)))
void err(int, const char*, ...);
__attribute__((__noreturn__, __format__(__printf__, 2, 3)))
void errx(int, const char*, ...);
__attribute__((__noreturn__, __format__(__printf__, 2, 0)))
void verr(int, const char*, va_list);
__attribute__((__noreturn__, __format__(__printf__, 2, 0)))
void verrx(int, const char*, va_list);
__attribute__((__format__(__printf__, 1, 2)))
void warn(const char*, ...);
__attribute__((__format__(__printf__, 1, 2)))
void warnx(const char*, ...);
__attribute__((__format__(__printf__, 1, 0)))
void vwarn(const char*, va_list);
__attribute__((__format__(__printf__, 1, 0)))
void vwarnx(const char*, va_list);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
