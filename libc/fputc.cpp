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

    fputc.cpp
    Writes a character to a FILE.

*******************************************************************************/

#include <stdio.h>

extern "C" int fputc(int c, FILE* fp)
{
	if ( fp->flags & _FILE_NO_BUFFER )
	{
		unsigned char c_char = c;
		if ( fwrite(&c_char, sizeof(c_char), 1, fp) != 1 )
			return EOF;
		return c;
	}

	if ( !fp->write_func )
		return EOF; // TODO: ferror doesn't report error!

	if ( fp->flags & _FILE_LAST_READ )
		fflush_stop_reading(fp);
	fp->flags |= _FILE_LAST_WRITE;

	if ( fp->amount_output_buffered == fp->buffersize && fflush(fp) != 0 )
		return EOF;

	fp->buffer[fp->amount_output_buffered++] = c;
	if ( c == '\n' && fflush(fp) != 0 )
		return EOF;

	return c;
}
