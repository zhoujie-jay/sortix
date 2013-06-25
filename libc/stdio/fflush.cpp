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

    stdio/fflush.cpp
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

	int mode = fp->flags & (_FILE_LAST_READ | _FILE_LAST_WRITE);
	if ( (mode & _FILE_LAST_READ) && fflush_stop_reading(fp) == EOF )
		return EOF;
	if ( (mode & _FILE_LAST_WRITE) && fflush_stop_writing(fp) == EOF )
		return EOF;
	fp->flags |= mode;
	return 0;
}
