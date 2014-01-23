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

    fcntl.h
    File control options.

*******************************************************************************/

#ifndef INCLUDE_FCNTL_H
#define INCLUDE_FCNTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sys/stat.h>

#include <sortix/fcntl.h>
#include <sortix/seek.h>

__BEGIN_DECLS

/* The kernel would like to simply deal with one bit for each base access mode,
   but using the traditional names O_RDONLY, O_WRONLY and O_RDWR for this would
   be weird, so it uses O_READ and O_WRITE bits instead. However, we provide the
   traditional names here instead to remain compatible. */
#define O_RDONLY O_READ
#define O_WRONLY O_WRITE
#define O_RDWR (O_READ | O_WRITE)

/* Backwards compatibility with existing systems that call it O_CREAT. */
#define O_CREAT O_CREATE

/* Compatibility with Linux and other systems that have this. */
#define O_ACCMODE (O_READ | O_WRITE | O_EXEC | O_SEARCH)

/* TODO: POSIX_FADV_* missing here */

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

int creat(const char* path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int open(const char* path, int oflag, ...);
int openat(int fd, const char* path, int oflag, ...);

__END_DECLS

#endif
