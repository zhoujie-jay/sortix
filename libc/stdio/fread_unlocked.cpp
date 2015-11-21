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

    stdio/fread_unlocked.cpp
    Reads data from a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C"
size_t fread_unlocked(void* ptr,
                      size_t element_size,
                      size_t num_elements,
                      FILE* fp)
{
	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	unsigned char* buf = (unsigned char*) ptr;
	size_t count = element_size * num_elements;
	if ( count == 0 )
		return num_elements;

	if ( fp->buffer_mode == _IONBF )
	{
		if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
			if ( fsetdefaultbuf_unlocked(fp) != 0 )
				return EOF;
		if ( !fp->read_func )
			return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, 0;
		if ( fp->flags & _FILE_LAST_WRITE )
			fflush_stop_writing_unlocked(fp);
		fp->flags |= _FILE_LAST_READ;
		fp->flags &= ~_FILE_STATUS_EOF;
		size_t sofar = 0;
		while ( sofar < count )
		{
			size_t request = count - sofar;
			if ( (size_t) SSIZE_MAX < request )
				request = SSIZE_MAX;
			ssize_t amount = fp->read_func(fp->user, buf + sofar, request);
			if ( amount < 0 )
				return fp->flags |= _FILE_STATUS_ERROR, sofar / num_elements;
			if ( amount == 0 )
				return fp->flags |= _FILE_STATUS_EOF, sofar / num_elements;
			sofar += amount;
		}
		return sofar / element_size;
	}

	for ( size_t n = 0; n < num_elements; n++ )
	{
		size_t offset = n * element_size;
		for ( size_t i = 0; i < element_size; i++ )
		{
			int c = fgetc_unlocked(fp);
			if ( c == EOF )
				return n;
			buf[offset + i] = (unsigned char) c;
		}
	}

	return num_elements;
}
