/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/__/types.h
    Data types.

*******************************************************************************/

#ifndef INCLUDE_SORTIX____TYPES_H
#define INCLUDE_SORTIX____TYPES_H

#include <features.h>
#include <__/stdint.h>
#include <__/wordsize.h>

__BEGIN_DECLS

typedef int __id_t;
typedef int __gid_t;
typedef int __uid_t;
typedef int __pid_t;
typedef unsigned int __mode_t;
typedef __intmax_t __off_t;
#define __OFF_MIN __INTMAX_MIN
#define __OFF_MAX __INTMAX_MAX
typedef unsigned int __useconds_t;
typedef __intmax_t __blksize_t;
typedef __intmax_t __blkcnt_t;
typedef unsigned int __nlink_t;
typedef __uintmax_t __ino_t;
typedef __uintptr_t __dev_t;
typedef __intmax_t __clock_t;
typedef int __clockid_t;
typedef long __time_t; /* TODO: Increase on 32-bit systems! */
typedef long __suseconds_t;
#if __WORDSIZE == 64
typedef long int __ssize_t;
#else
typedef int __ssize_t;
#endif
typedef __uintptr_t __timer_t;
typedef __SIZE_TYPE__ __socklen_t;

#if defined(SORTIX_KERNEL) || defined(LIBC_LIBRARY)
#define OFF_MIN __OFF_MIN
#endif
#if defined(SORTIX_KERNEL) || defined(LIBC_LIBRARY)
#define OFF_MAX __OFF_MAX
#endif

__END_DECLS

#endif
