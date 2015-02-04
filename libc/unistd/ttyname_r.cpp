/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    unistd/ttyname_r.cpp
    Returns the pathname of a terminal.

*******************************************************************************/

#include <errno.h>
#include <string.h>
#include <unistd.h>

extern "C" int ttyname_r(int fd, char* path, size_t path_size)
{
	if ( isatty(fd) < 1 )
		return -1;
	const char* result = "/dev/tty";
	if ( path_size <= strlcpy(path, result, path_size) )
		return errno = ERANGE, -1;
	return 0;
}
