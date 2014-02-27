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

    stdio/fdio.cpp
    Handles the file descriptor backend for the FILE* API.

*******************************************************************************/

#include <errno.h>
#include <unistd.h>

#include "fdio.h"

int fdio_reopen(void* user, const char* mode)
{
	(void) user;
	(void) mode;
	// TODO: Unfortunately, we don't support this yet. Note that we don't really
	// have to support this according to POSIX - but it'd be nicer to push this
	// restriction into the kernel and argue it's a security problem "What? No
	// you can't make this read-only descriptor writable!".
	return errno = ENOTSUP, -1;
}

ssize_t fdio_read(void* user, void* ptr, size_t size)
{
	struct fdio_state* fdio = (struct fdio_state*) user;
	return read(fdio->fd, ptr, size);
}

ssize_t fdio_write(void* user, const void* ptr, size_t size)
{
	struct fdio_state* fdio = (struct fdio_state*) user;
	return write(fdio->fd, ptr, size);
}

off_t fdio_seek(void* user, off_t offset, int whence)
{
	struct fdio_state* fdio = (struct fdio_state*) user;
	return lseek(fdio->fd, offset, whence);
}

int fdio_fileno(void* user)
{
	struct fdio_state* fdio = (struct fdio_state*) user;
	return fdio->fd;
}

int fdio_close(void* user)
{
	struct fdio_state* fdio = (struct fdio_state*) user;
	int result = close(fdio->fd);
	if ( fdio->free_indirect )
		fdio->free_indirect(fdio);
	return result;
}
