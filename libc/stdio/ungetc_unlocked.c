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

    stdio/ungetc_unlocked.c
    Inserts data in front of the current read queue, allowing applications to
    peek the incoming data and pretend they didn't.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>

int ungetc_unlocked(int c, FILE* fp)
{
	if ( c == EOF )
		return EOF;

	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing_unlocked(fp);

	fp->flags |= _FILE_LAST_READ;

	// TODO: Is this a bug that ungetc doesn't work for unbuffered files?
	if ( fp->buffer_mode == _IONBF )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	if ( fp->offset_input_buffer == 0 )
	{
		size_t amount = fp->amount_input_buffered - fp->offset_input_buffer;
		size_t offset = BUFSIZ - amount;
		if ( !offset )
			return EOF;
		memmove(fp->buffer + offset, fp->buffer, sizeof(fp->buffer[0]) * amount);
		fp->offset_input_buffer = offset;
		fp->amount_input_buffered = offset + amount;
	}

	fp->buffer[--fp->offset_input_buffer] = c;

	fp->flags &= ~_FILE_STATUS_EOF;

	return c;
}
