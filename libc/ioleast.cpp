/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    ioleast.cpp
    Versions of {,p}{read,write} that don't return until it has returned as much
    data as requested, end of file, or an error occurs. This is sometimes needed
    as read(2) and write(2) is not always guaranteed to fill up the entire
    buffer or write it all.

*******************************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#ifndef EEOF
#define EEOF EIO
#endif

extern "C" size_t readleast(int fd, void* buf, size_t least, size_t max)
{
	ssize_t amount = read(fd, buf, max);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { errno = EEOF; return 0; }
	if ( (size_t) amount < least )
	{
		void* nextbuf = (uint8_t*) buf + amount;
		size_t nextleast = least - amount;
		size_t nextmax = max - amount;
		amount += readleast(fd, nextbuf, nextleast, nextmax);
	}
	return amount;
}

extern "C" size_t readall(int fd, void* buf, size_t count)
{
	return readleast(fd, buf, count, count);
}

extern "C" size_t preadleast(int fd, void* buf, size_t least, size_t max,
                             off_t off)
{
	ssize_t amount = pread(fd, buf, max, off);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { errno = EEOF; return 0; }
	if ( (size_t) amount < least )
	{
		void* nextbuf = (uint8_t*) buf + amount;
		size_t nextleast = least - amount;
		size_t nextmax = max - amount;
		off_t nextoff = off + amount;
		amount += preadleast(fd, nextbuf, nextleast, nextmax, nextoff);
	}
	return amount;
}

extern "C" size_t preadall(int fd, void* buf, size_t count, off_t off)
{
	return preadleast(fd, buf, count, count, off);
}

extern "C" size_t writeleast(int fd, const void* buf, size_t least, size_t max)
{
	ssize_t amount = write(fd, buf, max);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { errno = EEOF; return 0; }
	if ( (size_t) amount < least )
	{
		const void* nextbuf = (const uint8_t*) buf + amount;
		size_t nextleast = least - amount;
		size_t nextmax = max - amount;
		amount += writeleast(fd, nextbuf, nextleast, nextmax);
	}
	return amount;
}

extern "C" size_t writeall(int fd, const void* buf, size_t count)
{
	return writeleast(fd, buf, count, count);
}

extern "C" size_t pwriteleast(int fd, const void* buf, size_t least, size_t max,
                              off_t off)
{
	ssize_t amount = pwrite(fd, buf, max, off);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { errno = EEOF; return 0; }
	if ( (size_t) amount < least )
	{
		const void* nextbuf = (const uint8_t*) buf + amount;
		size_t nextleast = least - amount;
		size_t nextmax = max - amount;
		off_t nextoff = off + amount;
		amount += pwriteleast(fd, nextbuf, nextleast, nextmax, nextoff);
	}
	return amount;
}

extern "C" size_t pwriteall(int fd, const void* buf, size_t count, off_t off)
{
	return pwriteleast(fd, buf, count, count, off);
}
