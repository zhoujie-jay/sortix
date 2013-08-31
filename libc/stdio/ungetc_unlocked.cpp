/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    stdio/ungetc_unlocked.cpp
    Inserts data in front of the current read queue, allowing applications to
    peek the incoming data and pretend they didn't.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>

extern "C" int ungetc_unlocked(int c, FILE* fp)
{
	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		if ( fsetdefaultbuf_unlocked(fp) != 0 )
			return EOF; // TODO: ferror doesn't report error!

	if ( !fp->read_func || fp->buffer_mode == _IONBF )
		return EOF;

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing_unlocked(fp);
	fp->flags |= _FILE_LAST_READ;

	if ( fp->offset_input_buffer == 0 )
	{
		size_t amount = fp->amount_input_buffered - fp->offset_input_buffer;
		size_t offset = fp->buffersize - amount;
		if ( !offset )
			return EOF;
		memmove(fp->buffer + offset, fp->buffer, sizeof(fp->buffer[0]) * amount);
		fp->offset_input_buffer = offset;
		fp->amount_input_buffered = offset + amount;
	}

	fp->buffer[--fp->offset_input_buffer] = c;
	return c;
}
