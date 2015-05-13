/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014, 2015.

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

    stdio_ext.h
    Extensions to stdio.h introduced by Solaris and glibc.

*******************************************************************************/

#ifndef _STDIO_EXT_H
#define _STDIO_EXT_H 1

#include <sys/cdefs.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t __fbufsize(FILE* fp);
size_t __fpending(FILE* fp);
void __fpurge(FILE* fp);
int __freadable(FILE* fp);
int __freading(FILE* fp);
void __fseterr(FILE* fp);
int __fwritable(FILE* fp);
int __fwriting(FILE* fp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
