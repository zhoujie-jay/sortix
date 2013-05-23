/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    ioleast.cpp
    Sortix functions for reliable reads and writes.

*******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "ioleast.h"

#if !defined(__sortix__)

size_t preadleast(int fd, void* buf, size_t least, size_t max, off_t off)
{
	ssize_t amount = pread(fd, buf, max, off);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { return 0; }
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

size_t preadall(int fd, void* buf, size_t count, off_t off)
{
	return preadleast(fd, buf, count, count, off);
}

size_t pwriteleast(int fd, const void* buf, size_t least, size_t max, off_t off)
{
	ssize_t amount = pwrite(fd, buf, max, off);
	if ( amount < 0 ) { return 0; }
	if ( least && !amount ) { return 0; }
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

size_t pwriteall(int fd, const void* buf, size_t count, off_t off)
{
	return pwriteleast(fd, buf, count, count, off);
}

#endif
