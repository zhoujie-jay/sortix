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

    stdio/fdio_install_path.cpp
    Opens a FILE from a path.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "fdio.h"

extern "C" bool fdio_install_path(FILE* fp, const char* path, const char* mode)
{
	int mode_flags = fparsemode(mode);
	if ( mode_flags < 0 )
		return false;

	if ( !(mode_flags & (FILE_MODE_READ | FILE_MODE_WRITE)) )
		return errno = EINVAL, false;

	int open_flags = 0;
	if ( mode_flags & FILE_MODE_READ ) open_flags |= O_READ;
	if ( mode_flags & FILE_MODE_WRITE ) open_flags |= O_WRITE;
	if ( mode_flags & FILE_MODE_APPEND ) open_flags |= O_APPEND;
	if ( mode_flags & FILE_MODE_CREATE ) open_flags |= O_CREATE;
	if ( mode_flags & FILE_MODE_TRUNCATE ) open_flags |= O_TRUNC;
	if ( mode_flags & FILE_MODE_EXCL ) open_flags |= O_EXCL;
	if ( mode_flags & FILE_MODE_CLOEXEC ) open_flags |= O_CLOEXEC;

	int fd = open(path, open_flags, 0666);
	if ( fd < 0 )
		return false;

	if ( !fdio_install_fd(fp, fd, mode) )
		return close(fd), false;

	return true;
}
