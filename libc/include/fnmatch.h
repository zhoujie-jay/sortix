/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    fnmatch.h
    Filename matching.

*******************************************************************************/

#ifndef INCLUDE_FNMATCH_H
#define INCLUDE_FNMATCH_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FNM_NOMATCH 1

#define FNM_PATHNAME (1 << 0)
#define FNM_NOESCAPE (1 << 1)
#define FNM_PERIOD (1 << 2)

int fnmatch(const char*, const char*, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
