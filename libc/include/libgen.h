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

    libgen.h
    What's left of some old pattern matching library.

*******************************************************************************/

#ifndef INCLUDE_LIBGEN_H
#define INCLUDE_LIBGEN_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

char* dirname(char* path);
char* basename(char* path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
