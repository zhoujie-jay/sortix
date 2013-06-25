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

    stdio/fileno.cpp
    Returns the underlying file descriptor of a FILE if applicable.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" int fileno(FILE* fp)
{
	int result = fp->fileno_func ? fp->fileno_func(fp->user) : -1;
	if ( result < 0 ) { errno = EBADF; }
	return result;
}