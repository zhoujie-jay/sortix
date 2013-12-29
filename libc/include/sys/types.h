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

    sys/types.h
    Data types.

*******************************************************************************/

// TODO: Make this header comply with POSIX-1.2008

#ifndef INCLUDE_SYS_TYPES_H
#define INCLUDE_SYS_TYPES_H

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

#ifndef __clock_t_defined
#define __clock_t_defined
typedef __clock_t clock_t;
#endif

#ifndef __clockid_t_defined
#define __clockid_t_defined
typedef __clockid_t clockid_t;
#endif

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif

#ifndef __fsblkcnt_t_defined
#define __fsblkcnt_t_defined
typedef __fsblkcnt_t fsblkcnt_t;
#endif

#ifndef __fsfilcnt_t_defined
#define __fsfilcnt_t_defined
typedef __fsfilcnt_t fsfilcnt_t;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __id_t_defined
#define __id_t_defined
typedef __id_t id_t;
#endif

#ifndef __ino_t_defined
#define __ino_t_defined
typedef __ino_t ino_t;
#endif

/* TODO: key_t */

#ifndef __mode_t_defined
#define __mode_t_defined
typedef __mode_t mode_t;
#endif

#ifndef __nlink_t_defined
#define __nlink_t_defined
typedef __nlink_t nlink_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif
#if defined(__is_sortix_kernel) || defined(__is_sortix_libc)
#define OFF_MIN __OFF_MIN
#define OFF_MAX __OFF_MAX
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif

#ifndef __suseconds_t_defined
#define __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifndef __timer_t_defined
#define __timer_t_defined
typedef __timer_t timer_t;
#endif

/* TODO: trace*_t */

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

#ifndef __useconds_t_defined
#define __useconds_t_defined
typedef __useconds_t useconds_t;
#endif

#if !defined(__is_sortix_kernel)
/* TODO: pthread*_t */
#endif

__END_DECLS

#endif
