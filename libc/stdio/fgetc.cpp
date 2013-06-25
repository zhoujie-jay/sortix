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

    stdio/fgetc.cpp
    Reads a single character from a FILE.

*******************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

extern "C" int fgetc(FILE* fp)
{
	if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
		if ( fsetdefaultbuf(fp) != 0 )
			return EOF; // TODO: ferror doesn't report error!

	if ( fp->buffer_mode == _IONBF )
	{
		unsigned char c;
		if ( fread(&c, sizeof(c), 1, fp) != 1 )
			return EOF;
		return c;
	}

	if ( !fp->read_func )
		return EOF; // TODO: ferror doesn't report error!

	if ( fp->flags & _FILE_LAST_WRITE )
		fflush_stop_writing(fp);
	fp->flags |= _FILE_LAST_READ;

	if ( fp->offset_input_buffer < fp->amount_input_buffered )
retry:
		return fp->buffer[fp->offset_input_buffer++];

	assert(fp->buffer && fp->buffersize);

	size_t pushback = _FILE_MAX_PUSHBACK;
	if ( fp->buffersize <= pushback )
		pushback = 0;
	size_t count = fp->buffersize - pushback;
	size_t size = sizeof(unsigned char);
	size_t numread = fp->read_func(fp->buffer + pushback, size, count, fp->user);
	if ( !numread )
		return EOF;

	fp->offset_input_buffer = pushback;
	fp->amount_input_buffered = pushback + numread;

	goto retry;
}
