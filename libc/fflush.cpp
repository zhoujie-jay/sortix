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

    fflush.cpp
    Flushes a FILE.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" int fflush(FILE* fp)
{
	if ( !fp )
	{
		int result = 0;
		for ( fp = _firstfile; fp; fp = fp->next ) { result |= fflush(fp); }
		return result;
	}
	if ( !fp->write_func ) { errno = EBADF; return EOF; }
	if ( !fp->bufferused ) { return 0; }
	size_t written = fp->write_func(fp->buffer, 1, fp->bufferused, fp->user);
	if ( written < fp->bufferused ) { return EOF; }
	fp->bufferused = 0;
	return 0;
}
