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

    stdio/fflush_stop_writing_unlocked.c
    Resets the FILE to a consistent state so it is ready for reading.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

int fflush_stop_writing_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_WRITE) )
		return 0;

	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !fp->write_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	int result = 0;

	size_t count = fp->amount_output_buffered;
	size_t sofar = 0;
	while ( sofar < count )
	{
		size_t request = count - sofar;
		if ( (size_t) SSIZE_MAX < request )
			request = SSIZE_MAX;
		ssize_t amount = fp->write_func(fp->user, fp->buffer + sofar, request);
		if ( amount < 0 )
		{
			fp->flags |= _FILE_STATUS_ERROR;
			result = EOF;
			break;
		}
		if ( amount == 0 )
		{
			fp->flags |= _FILE_STATUS_EOF;
			result = EOF;
			break;
		}
		sofar += amount;
	}

	fp->amount_output_buffered = 0;
	fp->flags &= ~_FILE_LAST_WRITE;

	return result;
}
