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

    stdio/fwrite_unlocked.cpp
    Writes data to a FILE.

*******************************************************************************/

#include <stdio.h>

extern "C"
size_t fwrite_unlocked(const void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( fp->buffer_mode == _IONBF )
	{
		if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
			if ( fsetdefaultbuf_unlocked(fp) != 0 )
				return EOF; // TODO: ferror doesn't report error!

		if ( !fp->write_func )
			return 0; // TODO: ferror doesn't report error!
		if ( fp->flags & _FILE_LAST_READ )
			fflush_stop_reading_unlocked(fp);
		fp->flags |= _FILE_LAST_WRITE;
		return fp->write_func(ptr, size, nmemb, fp->user);
	}

	const unsigned char* buf = (const unsigned char*) ptr;
	for ( size_t n = 0; n < nmemb; n++ )
	{
		size_t offset = n * size;
		for ( size_t i = 0; i < size; i++ )
		{
			size_t index = offset + i;
			if ( fputc_unlocked(buf[index], fp) == EOF )
				return n;
		}
	}

	return nmemb;
}
