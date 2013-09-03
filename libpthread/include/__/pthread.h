/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of Sortix libpthread.

    Sortix libpthread is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libpthread is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libpthread. If not, see <http://www.gnu.org/licenses/>.

    __/pthread.h
    Thread API.

*******************************************************************************/

#ifndef INCLUDE____PTHREAD_H
#define INCLUDE____PTHREAD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#define __sortix_libpthread__ 1

typedef int __pthread_attr_t;

typedef int __pthread_barrier_t;

typedef int __pthread_barrierattr_t;

#if defined(__is_sortix_libpthread)
typedef struct
{
	struct pthread_cond_elem* first;
	struct pthread_cond_elem* last;
} __pthread_cond_t;
#else
typedef struct
{
	void* __pthread_first;
	void* __pthread_last;
} __pthread_cond_t;
#endif

#if defined(__is_sortix_libpthread)
typedef struct
{
} __pthread_condattr_t;
#else
typedef struct
{
} __pthread_condattr_t;
#endif

typedef int __pthread_key_t;

#if defined(__is_sortix_libpthread)
typedef struct
{
	unsigned long lock;
	unsigned long type;
	unsigned long owner;
	unsigned long recursion;
} __pthread_mutex_t;
#else
typedef struct
{
	unsigned long __pthread_lock;
	unsigned long __pthread_type;
	unsigned long __pthread_owner;
	unsigned long __pthread_recursion;
} __pthread_mutex_t;
#endif

#if defined(__is_sortix_libpthread)
typedef struct
{
	int type;
} __pthread_mutexattr_t;
#else
typedef struct
{
	int __pthread_type;
} __pthread_mutexattr_t;
#endif

typedef int __pthread_once_t;

typedef int __pthread_rwlock_t;

typedef int __pthread_rwlockattr_t;

typedef int __pthread_spinlock_t;

#if defined(__is_sortix_libpthread)
typedef struct pthread* __pthread_t;
#else
typedef struct __pthread* __pthread_t;
#endif

__END_DECLS

#endif
