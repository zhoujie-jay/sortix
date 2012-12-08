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

    fflush_stop_writing.cpp
    Resets the FILE to a consistent state so it is ready for reading.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>

extern "C" int fflush_stop_writing(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_WRITE) )
		return 0;
	if ( !fp->write_func )
		return errno = EBADF, EOF;
	size_t size = sizeof(unsigned char);
	size_t count = fp->amount_output_buffered;
	int ret = 0;
	if ( fp->write_func(fp->buffer, size, count, fp->user) != count )
		ret = EOF; // TODO: Set errno!
	fp->amount_output_buffered = 0;
	fp->flags &= ~_FILE_LAST_WRITE;
	return ret;
}
