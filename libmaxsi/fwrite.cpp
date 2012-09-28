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

	fwrote.cpp
	Writes data to a FILE.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( !fp->write_func ) { errno = EBADF; return 0; }
	fp->flags &= ~_FILE_LAST_READ; fp->flags |= _FILE_LAST_WRITE;
	char* str = (char*) ptr;
	size_t total = size * nmemb;
	size_t sofar = 0;
	while ( sofar < total )
	{
		size_t left = total - sofar;
		if ( (!fp->bufferused && fp->buffersize <= left) ||
		     (fp->flags & _FILE_NO_BUFFER) )
		{
			return sofar + fp->write_func(str + sofar, 1, left, fp->user);
		}

		size_t available = fp->buffersize - fp->bufferused;
		size_t count = ( left < available ) ? left : available;
		count = left;
		for ( size_t i = 0; i < count; i++ )
		{
			char c = str[sofar++];
			fp->buffer[fp->bufferused++] = c;
			if ( c == '\n' )
			{
				if ( fflush(fp) ) { return sofar; }
				break;
			}
		}

		if ( fp->buffersize <= fp->bufferused )
		{
			if ( fflush(fp) ) { return sofar; }
		}
	}
	return sofar;
}
