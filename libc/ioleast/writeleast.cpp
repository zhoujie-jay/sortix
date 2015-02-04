/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2015.

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

    ioleast/writeleast.cpp
    Writes at least as much data as requested or more.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <ioleast.h>
#include <stdint.h>
#include <unistd.h>

extern "C"
size_t writeleast(int fd, const void* buf_ptr, size_t least, size_t max)
{
	assert(least <= max);
	const unsigned char* buf = (const unsigned char*) buf_ptr;
	size_t done = 0;
	do
	{
		ssize_t amount = write(fd, buf + done, max - done);
		if ( amount < 0 )
			return done;
		if ( !amount && done < least )
			return errno = EEOF, done;
		if ( !amount )
			break;
		done += amount;
	} while ( done < least );
	return done;
}
