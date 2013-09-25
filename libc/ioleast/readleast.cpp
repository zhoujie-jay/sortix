/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    ioleast/readleast.cpp
    Reads at least as much data as requested or more.

*******************************************************************************/

#include <errno.h>
#include <ioleast.h>
#include <stdint.h>
#include <unistd.h>

extern "C" size_t readleast(int fd, void* buf, size_t least, size_t max)
{
	ssize_t amount = read(fd, buf, max);
	if ( amount < 0 )
		return 0;
	if ( least && !amount )
		return errno = EEOF, 0;
	if ( (size_t) amount < least )
	{
		void* nextbuf = (uint8_t*) buf + amount;
		size_t nextleast = least - amount;
		size_t nextmax = max - amount;
		amount += readleast(fd, nextbuf, nextleast, nextmax);
	}
	return amount;
}
