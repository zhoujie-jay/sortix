/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    stdio/fdio.h
    Handles the file descriptor backend for the FILE* API.

*******************************************************************************/

#ifndef STDIO_FDIO_H
#define STDIO_FDIO_H

#include <sys/cdefs.h>

#include <sys/stat.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stdio.h>

__BEGIN_DECLS

struct fdio_state
{
	void (*free_indirect)(void*);
	int fd;
};

int fdio_reopen(void* user, const char* mode);
ssize_t fdio_read(void* user, void* ptr, size_t size);
ssize_t fdio_write(void* user, const void* ptr, size_t size);
off_t fdio_seek(void* user, off_t offset, int whence);
int fdio_fileno(void* user);
int fdio_close(void* user);

bool fdio_install_fd(FILE* fp, int fd, const char* mode);
bool fdio_install_path(FILE* fp, const char* path, const char* mode);

__END_DECLS

#endif
