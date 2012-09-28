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

	fread.cpp
	Reads data from a FILE.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" size_t fread(void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( fp->numpushedback && size != 1 ) { errno = ENOSYS; return 0; }
	if ( fp->numpushedback && nmemb )
	{
		unsigned char* buf = (unsigned char*) ptr;
		size_t amount = nmemb < fp->numpushedback ? nmemb : fp->numpushedback;
		for ( size_t i = 0; i < amount; i++ )
		{
			buf[i] = fp->pushedback[--(fp->numpushedback)];
		}
		if ( nmemb <= amount ) { return nmemb; }
		return amount + fread(buf + amount, size, nmemb - amount, fp);
	}
	if ( !fp->read_func ) { errno = EBADF; return 0; }
	fp->flags &= ~_FILE_LAST_WRITE; fp->flags |= _FILE_LAST_READ;
	return fp->read_func(ptr, size, nmemb, fp->user);
}
