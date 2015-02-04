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

    ioleast.h
    Versions of {,p}{read,write} that don't return until it has returned as much
    data as requested, end of file, or an error occurs. This is sometimes needed
    as read(2) and write(2) is not always guaranteed to fill up the entire
    buffer or write it all.

*******************************************************************************/

#ifndef SORTIX_COMPATIBILITY_INCLUDE_IOLEAST_H
#define SORTIX_COMPATIBILITY_INCLUDE_IOLEAST_H

#if defined(__sortix__) || defined(__sortix_libc__)

#include_next <ioleast.h>

#else

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#if !defined(EEOF) && defined(EIO)
#define EEOF EIO
#endif

__attribute__((unused)) static inline
size_t readleast(int fd, void* buf_ptr, size_t least, size_t max)
{
	assert(least <= max);
	unsigned char* buf = (unsigned char*) buf_ptr;
	size_t done = 0;
	do
	{
		ssize_t amount = read(fd, buf + done, max - done);
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

__attribute__((unused)) static inline
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

__attribute__((unused)) static inline
size_t preadleast(int fd, void* buf_ptr, size_t least, size_t max, off_t off)
{
	assert(least <= max);
	unsigned char* buf = (unsigned char*) buf_ptr;
	size_t done = 0;
	do
	{
		ssize_t amount = pread(fd, buf + done, max - done, off + done);
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

__attribute__((unused)) static inline
size_t pwriteleast(int fd, const void* buf_ptr, size_t least, size_t max, off_t off)
{
	assert(least <= max);
	const unsigned char* buf = (const unsigned char*) buf_ptr;
	size_t done = 0;
	do
	{
		ssize_t amount = pwrite(fd, buf + done, max - done, off + done);
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

__attribute__((unused)) static inline
size_t readall(int fd, void* buf, size_t count)
{
	return readleast(fd, buf, count, count);
}

__attribute__((unused)) static inline
size_t writeall(int fd, const void* buf, size_t count)
{
	return writeleast(fd, buf, count, count);
}

__attribute__((unused)) static inline
size_t preadall(int fd, void* buf, size_t count, off_t off)
{
	return preadleast(fd, buf, count, count, off);
}

__attribute__((unused)) static inline
size_t pwriteall(int fd, const void* buf, size_t count, off_t off)
{
	return pwriteleast(fd, buf, count, count, off);
}

#endif

#endif
