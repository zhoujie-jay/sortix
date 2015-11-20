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

    stdio/fdio_install_fd.cpp
    Opens a FILE from a file descriptor.

*******************************************************************************/

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fdio.h"

extern "C" bool fdio_install_fd(FILE* fp, int fd, const char* mode)
{
	int mode_flags = fparsemode(mode);
	if ( mode_flags < 0 )
		return false;

	if ( !(mode_flags & (FILE_MODE_READ | FILE_MODE_WRITE)) )
		return errno = EINVAL, false;

	struct stat st;
	if ( fstat(fd, &st) == 0 &&
	     (mode_flags & FILE_MODE_WRITE) &&
	     S_ISDIR(st.st_mode) )
		return errno = EISDIR, false;

	if ( mode_flags & FILE_MODE_READ )
		fp->flags |= _FILE_READABLE;
	if ( mode_flags & FILE_MODE_WRITE )
		fp->flags |= _FILE_WRITABLE;
	fp->fd = fd;
	fp->user = fp;
	fp->read_func = fdio_read;
	fp->write_func = fdio_write;
	fp->seek_func = fdio_seek;
	fp->close_func = fdio_close;

	return true;
}
