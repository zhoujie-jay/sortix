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

    sys/stat.h
    Data returned by the stat() function.

*******************************************************************************/

#ifndef INCLUDE_SYS_STAT_H
#define INCLUDE_SYS_STAT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __blkcnt_t_defined
#define __blkcnt_t_defined
typedef __blkcnt_t blkcnt_t;
#endif

#ifndef __blksize_t_defined
#define __blksize_t_defined
typedef __blksize_t blksize_t;
#endif

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif

#ifndef __ino_t_defined
#define __ino_t_defined
typedef __ino_t ino_t;
#endif

#ifndef __mode_t_defined
#define __mode_t_defined
typedef __mode_t mode_t;
#endif

#ifndef __nlink_t_defined
#define __nlink_t_defined
typedef __nlink_t nlink_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

__END_DECLS

#include <sortix/timespec.h>
#include <sortix/stat.h>

__BEGIN_DECLS

/* POSIX mandates that we define these compatibility macros to support programs
   that are yet to embrace struct timespec. Nofify users that use the old names
   by inserting macros that insert annoying warnings. If you really need the
   old names to be compatible with older systems, you can check whether these
   names are defined as macros and use the newer names and otherwise using these
   older names. */
#define st_atime __PRAGMA_WARNING("st_atime is deprecated in favor of st_atim.tv_sec") st_atim.tv_sec
#define st_ctime __PRAGMA_WARNING("st_ctime is deprecated in favor of st_ctim.tv_sec") st_ctim.tv_sec
#define st_mtime __PRAGMA_WARNING("st_mtime is deprecated in favor of st_mtim.tv_sec") st_mtim.tv_sec

int chmod(const char* path, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char* path, mode_t mode, int flags);
int fstat(int fd, struct stat* st);
int fstatat(int dirfd, const char* path, struct stat* buf, int flags);
int futimens(int fd, const struct timespec times[2]);
int lstat(const char* __restrict path, struct stat* __restrict st);
mode_t getumask(void);
int mkdir(const char* path, mode_t mode);
int mkdirat(int dirfd, const char* path, mode_t mode);
/* TODO: mkfifo */
/* TODO: mkfifoat */
/* TODO: mknod? */
/* TODO: mknodat? */
int stat(const char* __restrict path, struct stat* __restrict st);
mode_t umask(mode_t mask);
int utimens(const char* path, const struct timespec times[2]);
int utimensat(int dirfd, const char* path, const struct timespec times[2],
              int flags);

__END_DECLS

#endif
