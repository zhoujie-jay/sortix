/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    stdio/fflush_stop_reading_unlocked.c
    Resets the FILE to a consistent state ready for writing.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int fflush_stop_reading_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_READ) )
		return 0;

	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	int saved_errno = errno;
	off_t my_pos = -1;
	if ( fp->seek_func )
		my_pos = fp->seek_func(fp->user, 0, SEEK_CUR);
	errno = saved_errno;

	int ret = 0;
	if ( 0 <= my_pos )
	{
		size_t buffer_ahead = fp->amount_input_buffered - fp->offset_input_buffer;
		off_t expected_pos = (uintmax_t) my_pos < (uintmax_t) buffer_ahead ? 0 :
		                     my_pos - buffer_ahead;
		if ( 0 <= my_pos && fp->seek_func(fp->user, expected_pos, SEEK_SET) < 0 )
			fp->flags |= _FILE_STATUS_ERROR, ret = EOF;
	}

	fp->amount_input_buffered = fp->offset_input_buffer = 0;
	fp->flags &= ~_FILE_LAST_READ;

	return ret;
}
