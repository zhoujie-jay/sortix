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

    errno.h
    System error numbers.

*******************************************************************************/

#ifndef INCLUDE_ERRNO_H
#define INCLUDE_ERRNO_H

#include <features.h>

__BEGIN_DECLS

#define ENOTBLK 12
#define ENODEV 13
#define EWOULDBLOCK 14
#define EBADF 15
#define EOVERFLOW 16
#define ENOENT 17
#define ENOSPC 18
#define EEXIST 19
#define EROFS 20
#define EINVAL 21
#define ENOTDIR 22
#define ENOMEM 23
#define ERANGE 24
#define EISDIR 25
#define EPERM 26
#define EIO 27
#define ENOEXEC 28
#define EACCES 29
#define ESRCH 30
#define ENOTTY 31
#define ECHILD 32
#define ENOSYS 33
#define ENOTSUP 34
#define EBLOCKING 35
#define EINTR 36
#define ENOTEMPTY 37
#define EBUSY 38
#define EPIPE 39
#define EILSEQ 40
#define ELAKE 41
#define EMFILE 42
#define EAGAIN 43
#define EEOF 44
#define EBOUND 45
#define EINIT 46
#define ENODRV 47
#define E2BIG 48
#define EFBIG 49
#define EXDEV 50
#define ESPIPE 51
#define ENAMETOOLONG 52
#define ELOOP 53
#define EMLINK 54
#define ENXIO 55
#define EPROTONOSUPPORT 56
#define EAFNOSUPPORT 57
#define ENOTSOCK 58
#define EADDRINUSE 59
#define ETIMEDOUT 60
#define ECONNREFUSED 61
#define EDOM 62
#define EINPROGRESS 63
#define EALREADY 64
#define ESHUTDOWN 65
#define ECONNABORTED 66
#define ECONNRESET 67
#define EADDRNOTAVAIL 68
#define EISCONN 69
#define EFAULT 70
#define EDESTADDRREQ 71
#define EHOSTUNREACH 72
#define EMSGSIZE 73
#define ENETDOWN 74
#define ENETRESET 75
#define ENETUNREACH 76
#define ENOBUFS 77
#define ENOMSG 78
#define ENOPROTOOPT 79
#define ENOTCONN 80
#define EDEADLK 81
#define ENFILE 82
#define EPROTOTYPE 83
#define ENOLCK 84

#define EOPNOTSUPP ENOTSUP

/* Returns the address of the errno variable for this thread. */
int* get_errno_location(void);

/* get_errno_location will forward the request to a user-specified function if
   specified, or if NULL, will return the global thread-shared errno value. */
typedef int* (*errno_location_func_t)(void);
void set_errno_location_func(errno_location_func_t func);

#define errno (*get_errno_location())

extern char* program_invocation_name;
extern char* program_invocation_short_name;

/* Satisfy broken programs that expect these to be macros. */
#define program_invocation_name program_invocation_name
#define program_invocation_short_name program_invocation_short_name

__END_DECLS

#endif
