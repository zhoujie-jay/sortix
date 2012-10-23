/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

/* TODO: Make this header comply with POSIX-1.2008, if it makes sense. */

#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#include <features.h>

__BEGIN_DECLS
@include(blkcnt_t.h)
@include(blksize_t.h)
@include(dev_t.h)
@include(ino_t.h)
@include(mode_t.h)
@include(nlink_t.h)
@include(uid_t.h)
@include(gid_t.h)
@include(off_t.h)
@include(time_t.h)
__END_DECLS

#include <sortix/stat.h>

__BEGIN_DECLS

int chmod(const char* path, mode_t mode);
int fchmod(int fd, mode_t mode);
int fstat(int fd, struct stat* st);
int fstatat(int dirfd, const char* path, struct stat* buf, int flags);
int lstat(const char* restrict path, struct stat* restrict st);
int mkdir(const char* path, mode_t mode);
int mkdirat(int dirfd, const char* path, mode_t mode);
int stat(const char* restrict path, struct stat* restrict st);
mode_t umask(mode_t mask);

__END_DECLS

#endif
