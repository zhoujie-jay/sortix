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

    stdio/fwrite_unlocked.c
    Writes data to a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

size_t fwrite_unlocked(const void* ptr,
                       size_t element_size,
                       size_t num_elements,
                       FILE* fp)
{
	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, -1;

	const unsigned char* buf = (const unsigned char*) ptr;
	size_t count = element_size * num_elements;
	if ( count == 0 )
		return num_elements;

	if ( fp->buffer_mode == _IONBF )
	{
		if ( !(fp->flags & _FILE_BUFFER_MODE_SET) )
			setvbuf_unlocked(fp, NULL, fp->buffer_mode, 0);
		if ( !fp->write_func )
			return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, 0;
		if ( fp->flags & _FILE_LAST_READ )
			fflush_stop_reading_unlocked(fp);
		fp->flags |= _FILE_LAST_WRITE;
		fp->flags &= ~_FILE_STATUS_EOF;
		size_t sofar = 0;
		while ( sofar < count )
		{
			size_t request = count - sofar;
			if ( (size_t) SSIZE_MAX < request )
				request = SSIZE_MAX;
			ssize_t amount = fp->write_func(fp->user, buf + sofar, request);
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
			size_t index = offset + i;
			if ( fputc_unlocked(buf[index], fp) == EOF )
				return n;
		}
	}

	return num_elements;
}
