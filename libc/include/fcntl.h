/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef INCLUDE_FCNTL_H
#define INCLUDE_FCNTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sys/stat.h>

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

/* TODO: F_* missing here */

/* TODO: F_RDLCK, F_UNLCK, F_WRLCK missing here */

/* TODO: AT_SYMLINK_FOLLOW missing here */

/* TODO: POSIX_FADV_* missing here */

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

struct flock
{
	short l_type; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	short l_whence; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	off_t l_start; /* Relative offset in bytes. */
	off_t l_len; /* Size; if 0 then until EOF. */
	pid_t l_pid; /* Process ID of the process holding the lock; returned with F_GETLK. */
};

int creat(const char* path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int open(const char* path, int oflag, ...);
int openat(int fd, const char* path, int oflag, ...);

__END_DECLS

#endif
