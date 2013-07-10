/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

#ifndef _FCNTL_H
#define _FCNTL_H 1

#include <features.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sys/stat.h>

__BEGIN_DECLS

/* TODO: F_* missing here */

/* TODO: F_RDLCK, F_UNLCK, F_WRLCK missing here */

/* TODO: AT_FDCWD missing here */
/* TODO: AT_EACCESS missing here */
/* TODO: AT_SYMLINK_NOFOLLOW missing here */
/* TODO: AT_SYMLINK_FOLLOW missing here */
/* TODO: AT_REMOVEDIR missing here */

/* TODO: POSIX_FADV_* missing here */

@include(pid_t.h)

struct flock
{
	short l_type; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	short l_whence; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	off_t l_start; /* Relative offset in bytes. */
	off_t l_len; /* Size; if 0 then until EOF. */
	pid_t l_pid; /* Process ID of the process holding the lock; returned with F_GETLK. */
};

int fcntl(int fd, int cmd, ...);
int open(const char* path, int oflag, ...);
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
int creat(const char* path, mode_t mode);
int openat(int fd, const char* path, int oflag, ...);
#endif

__END_DECLS

#endif
