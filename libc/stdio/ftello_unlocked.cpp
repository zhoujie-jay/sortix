/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    stdio/ftello_unlocked.cpp
    Returns the current offset of a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>

extern "C" off_t ftello_unlocked(FILE* fp)
{
	if ( !fp->seek_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, -1;
	off_t offset = fp->seek_func(fp->user, 0, SEEK_CUR);
	if ( offset < 0 )
		return -1;
	off_t read_ahead = fp->amount_input_buffered - fp->offset_input_buffer;
	off_t write_behind = fp->amount_output_buffered;
	if ( offset < read_ahead + write_behind ) // Too much ungetc'ing.
		return 0;
	return offset - read_ahead + write_behind;
}
