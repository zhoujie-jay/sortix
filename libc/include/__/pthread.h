/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * __/pthread.h
 * Thread API.
 */

#ifndef INCLUDE____PTHREAD_H
#define INCLUDE____PTHREAD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__is_sortix_libc)
typedef struct
{
	__SIZE_TYPE__ stack_size;
	int detach_state;
} __pthread_attr_t;
#else
typedef struct
{
	__SIZE_TYPE__ __pthread_stack_size;
	int __pthread_detached_state;
} __pthread_attr_t;
#endif

typedef int __pthread_barrier_t;

typedef int __pthread_barrierattr_t;

#if defined(__is_sortix_libc)
typedef struct
{
	struct pthread_cond_elem* first;
	struct pthread_cond_elem* last;
	__clock_t clock;
} __pthread_cond_t;
#else
typedef struct
{
	void* __pthread_first;
	void* __pthread_last;
	__clock_t __pthread_clock;
} __pthread_cond_t;
#endif

#if defined(__is_sortix_libc)
typedef struct
{
	__clock_t clock;
} __pthread_condattr_t;
#else
typedef struct
{
	__clock_t __pthread_clock;
} __pthread_condattr_t;
#endif

typedef __SIZE_TYPE__ __pthread_key_t;

#if defined(__is_sortix_libc)
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

#if defined(__is_sortix_libc)
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

#if defined(__is_sortix_libc)
typedef struct
{
	__pthread_mutex_t lock;
	int executed;
} __pthread_once_t;
#else
typedef struct
{
	__pthread_mutex_t __pthread_lock;
	int __pthread_executed;
} __pthread_once_t;
#endif

#if defined(__is_sortix_libc)
typedef struct
{
	__pthread_cond_t reader_condition;
	__pthread_cond_t writer_condition;
	__pthread_mutex_t request_mutex;
	unsigned long num_readers;
	unsigned long num_writers;
	unsigned long pending_readers;
	unsigned long pending_writers;
} __pthread_rwlock_t;
#else
typedef struct
{
	__pthread_cond_t __pthread_reader_condition;
	__pthread_cond_t __pthread_writer_condition;
	__pthread_mutex_t __pthread_request_mutex;
	unsigned long __pthread_num_readers;
	unsigned long __pthread_num_writers;
	unsigned long __pthread_pending_readers;
	unsigned long __pthread_pending_writers;
} __pthread_rwlock_t;
#endif

#if defined(__is_sortix_libc)
typedef struct
{
} __pthread_rwlockattr_t;
#else
typedef struct
{
} __pthread_rwlockattr_t;
#endif

typedef int __pthread_spinlock_t;

#if defined(__is_sortix_libc)
typedef struct pthread* __pthread_t;
#else
typedef struct __pthread* __pthread_t;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
