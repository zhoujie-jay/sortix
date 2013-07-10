/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

#include <features.h>
#include <stdio.h>

enum
{
	FSETLOCKING_QUERY = 0,
	FSETLOCKING_INTERNAL,
	FSETLOCKING_BYCALLER,
};

#define FSETLOCKING_QUERY FSETLOCKING_QUERY
#define FSETLOCKING_INTERNAL FSETLOCKING_INTERNAL
#define FSETLOCKING_BYCALLER FSETLOCKING_BYCALLER

__BEGIN_DECLS

extern size_t __fbufsize(FILE* fp);
extern int __freading(FILE* fp);
extern int __fwriting(FILE* fp);
extern int __freadable(FILE* fp);
extern int __fwritable(FILE* fp);
extern int __flbf(FILE* fp);
extern void __fpurge(FILE* fp);
extern size_t __fpending(FILE* fp);
extern void _flushlbf(void);
extern int __fsetlocking(FILE* fp, int type);

__END_DECLS

#endif
