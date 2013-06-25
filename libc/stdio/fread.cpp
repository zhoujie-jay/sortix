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

    stdio/fread.cpp
    Reads data from a FILE.

*******************************************************************************/

#include <stdio.h>

extern "C" size_t fread(void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( fp->buffer_mode == _IONBF )
	{
		if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
			if ( fsetdefaultbuf(fp) != 0 )
				return EOF; // TODO: ferror doesn't report error!
		if ( !fp->read_func )
			return 0; // TODO: ferror doesn't report error!
		if ( fp->flags & _FILE_LAST_WRITE )
			fflush_stop_writing(fp);
		fp->flags |= _FILE_LAST_READ;
		return fp->read_func(ptr, size, nmemb, fp->user);
	}

	unsigned char* buf = (unsigned char*) ptr;
	for ( size_t n = 0; n < nmemb; n++ )
	{
		size_t offset = n * size;
		for ( size_t i = 0; i < size; i++ )
		{
			int c = fgetc(fp);
			if ( c == EOF )
				return n;
			size_t index = i + offset;
			buf[index] = c;
		}
	}

	return nmemb;
}
