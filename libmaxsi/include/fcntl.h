/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	fcntl.h
	File control options.

******************************************************************************/

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef	_FCNTL_H
#define	_FCNTL_H 1

#include <features.h>

__BEGIN_DECLS

/* TODO: F_* missing here */

/* TODO: FD_CLOEXEC missing here */

/* TODO: F_RDLCK, F_UNLCK, F_WRLCK missing here */

@include(SEEK_SET.h)
@include(SEEK_CUR.h)
@include(SEEK_END.h)

/* TODO: Keep these aligned with those in the Sortix kernel */
#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3
#define O_EXEC 4
#define O_SEARCH 5
#define O_APPEND (1<<3)
#define O_CLOEXEC (1<<4)
#define O_CREAT (1<<5)
#define O_DIRECTORY (1<<6)
#define O_DSYNC (1<<6)
#define O_EXCL (1<<7)
#define O_NOCTTY (1<<8)
#define O_NOFOLLOW (1<<9)
#define O_RSYNC (1<<11)
#define O_SYNC (1<<12)
#define O_TRUNC (1<<13)
#define O_TTY_INIT (1<<13)

@include(mode_t.h)
@include(mode_t_values.h)

/* TODO: AT_FDCWD missing here */
/* TODO: AT_EACCESS missing here */
/* TODO: AT_SYMLINK_NOFOLLOW missing here */
/* TODO: AT_SYMLINK_FOLLOW missing here */
/* TODO: AT_REMOVEDIR missing here */

/* TODO: POSIX_FADV_* missing here */

@include(pid_t.h)

struct _flock
{
	short l_type; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	short l_whence; /* Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
	off_t l_start; /* Relative offset in bytes. */
	off_t l_len; /* Size; if 0 then until EOF. */
	pid_t l_pid; /* Process ID of the process holding the lock; returned with F_GETLK. */
};

typedef struct _flock flock;

/* TODO: These are not implemented in libmaxsi/sortix yet. */
int open(const char* path, int oflag, ...);
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
int creat(const char* path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int openat(int fd, const char* path, int oflag, ...);
#endif

__END_DECLS

#endif
