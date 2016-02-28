/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    stdio/fgetc_unlocked.c
    Reads a single character from a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int fgetc_unlocked(FILE* fp)
{
	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);

	if ( fp->buffer_mode == _IONBF )
	{
		unsigned char c;
		if ( fread_unlocked(&c, sizeof(c), 1, fp) != 1 )
			return EOF;
		return (int) c;
	}

	if ( !fp->read_func )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing_unlocked(fp);

	fp->flags |= _FILE_LAST_READ;
	fp->flags &= ~_FILE_STATUS_EOF;

	if ( !(fp->offset_input_buffer < fp->amount_input_buffered) )
	{
		assert(fp->buffer && BUFSIZ);

		size_t pushback = _FILE_MAX_PUSHBACK;
		if ( BUFSIZ <= pushback )
			pushback = 0;
		size_t count = BUFSIZ - pushback;
		if ( (size_t) SSIZE_MAX < count )
			count = SSIZE_MAX;
		ssize_t numread = fp->read_func(fp->user, fp->buffer + pushback, count);
		if ( numread < 0 )
			return fp->flags |= _FILE_STATUS_ERROR, EOF;
		if ( numread == 0 )
			return fp->flags |= _FILE_STATUS_EOF, EOF;

		fp->offset_input_buffer = pushback;
		fp->amount_input_buffered = pushback + numread;
	}

	return fp->buffer[fp->offset_input_buffer++];
}
