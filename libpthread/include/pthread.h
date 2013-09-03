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

    pthread.h
    Thread API.

*******************************************************************************/

#ifndef INCLUDE_PTHREAD_H
#define INCLUDE_PTHREAD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sched.h>
#include <time.h>

#if defined(__is_sortix_libpthread)
#include <stddef.h>
#include <stdint.h>
#endif

#if defined(__is_sortix_libpthread)
#include <sortix/uthread.h>
#endif

#include <__/pthread.h>

__BEGIN_DECLS

/* TODO: #define PTHREAD_BARRIER_SERIAL_THREAD */
/* TODO: #define PTHREAD_CANCEL_ASYNCHRONOUS */
/* TODO: #define PTHREAD_CANCEL_ENABLE */
/* TODO: #define PTHREAD_CANCEL_DEFERRED */
/* TODO: #define PTHREAD_CANCEL_DISABLE */
/* TODO: #define PTHREAD_CANCELED */
/* TODO: #define PTHREAD_CREATE_DETACHED */
/* TODO: #define PTHREAD_CREATE_JOINABLE */
/* TODO: #define PTHREAD_EXPLICIT_SCHED */
/* TODO: #define PTHREAD_INHERIT_SCHED */
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
/* TODO: #define PTHREAD_MUTEX_ERRORCHECK */
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
/* TODO: #define PTHREAD_MUTEX_ROBUST */
/* TODO: #define PTHREAD_MUTEX_STALLED */
/* TODO: #define PTHREAD_ONCE_INIT */
/* TODO: #define PTHREAD_PRIO_INHERIT */
/* TODO: #define PTHREAD_PRIO_NONE */
/* TODO: #define PTHREAD_PRIO_PROTECT */
/* TODO: #define PTHREAD_PROCESS_SHARED */
/* TODO: #define PTHREAD_PROCESS_PRIVATE */
/* TODO: #define PTHREAD_SCOPE_PROCESS */
/* TODO: #define PTHREAD_SCOPE_SYSTEM */

#ifndef __pthread_attr_t_defined
#define __pthread_attr_t_defined
typedef __pthread_attr_t pthread_attr_t;
#endif

#ifndef __pthread_barrier_t_defined
#define __pthread_barrier_t_defined
typedef __pthread_barrier_t pthread_barrier_t;
#endif

#ifndef __pthread_barrierattr_t_defined
#define __pthread_barrierattr_t_defined
typedef __pthread_barrierattr_t pthread_barrierattr_t;
#endif

#ifndef __pthread_cond_t_defined
#define __pthread_cond_t_defined
typedef __pthread_cond_t pthread_cond_t;
#endif

#ifndef __pthread_condattr_t_defined
#define __pthread_condattr_t_defined
typedef __pthread_condattr_t pthread_condattr_t;
#endif

#ifndef __pthread_key_t_defined
#define __pthread_key_t_defined
typedef __pthread_key_t pthread_key_t;
#endif

#ifndef __pthread_mutex_t_defined
#define __pthread_mutex_t_defined
typedef __pthread_mutex_t pthread_mutex_t;
#endif

#ifndef __pthread_mutexattr_t_defined
#define __pthread_mutexattr_t_defined
typedef __pthread_mutexattr_t pthread_mutexattr_t;
#endif

#ifndef __pthread_once_t_defined
#define __pthread_once_t_defined
typedef __pthread_once_t pthread_once_t;
#endif

#ifndef __pthread_rwlock_t_defined
#define __pthread_rwlock_t_defined
typedef __pthread_rwlock_t pthread_rwlock_t;
#endif

#ifndef __pthread_rwlockattr_t_defined
#define __pthread_rwlockattr_t_defined
typedef __pthread_rwlockattr_t pthread_rwlockattr_t;
#endif

#ifndef __pthread_spinlock_t_defined
#define __pthread_spinlock_t_defined
typedef __pthread_spinlock_t pthread_spinlock_t;
#endif

#ifndef __pthread_t_defined
#define __pthread_t_defined
typedef __pthread_t pthread_t;
#endif

#if defined(__is_sortix_libpthread)
struct pthread
{
	struct uthread uthread;
};
#endif

#if defined(__is_sortix_libpthread)
struct pthread_cond_elem
{
	struct pthread_cond_elem* next;
	volatile unsigned long woken;
};
#endif

#define PTHREAD_COND_INITIALIZER { NULL, NULL, CLOCK_REALTIME }
#define PTHREAD_MUTEX_INITIALIZER { 0, PTHREAD_MUTEX_DEFAULT, 0, 0 }
#define PTHREAD_RWLOCK_INITIALIZER { PTHREAD_COND_INITIALIZER, \
                                     PTHREAD_COND_INITIALIZER, \
                                     PTHREAD_MUTEX_INITIALIZER, 0, 0, 0, 0 }

#define PTHREAD_NORMAL_MUTEX_INITIALIZER_NP { 0, PTHREAD_MUTEX_NORMAL, 0, 0 }
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP { 0, PTHREAD_MUTEX_RECURSIVE, 0, 0 }

void pthread_initialize(void);

/* TODO: pthread_atfork */
/* TODO: pthread_attr_destroy */
/* TODO: pthread_attr_getdetachstate */
/* TODO: pthread_attr_getguardsize */
/* TODO: pthread_attr_getinheritsched */
/* TODO: pthread_attr_getschedparam */
/* TODO: pthread_attr_getschedpolicy */
/* TODO: pthread_attr_getscope */
/* TODO: pthread_attr_getstack */
/* TODO: pthread_attr_getstacksize */
/* TODO: pthread_attr_init */
/* TODO: pthread_attr_setdetachstate */
/* TODO: pthread_attr_setguardsize */
/* TODO: pthread_attr_setinheritsched */
/* TODO: pthread_attr_setschedparam */
/* TODO: pthread_attr_setschedpolicy */
/* TODO: pthread_attr_setscope */
/* TODO: pthread_attr_setstack */
/* TODO: pthread_attr_setstacksize */
/* TODO: pthread_barrier_destroy */
/* TODO: pthread_barrier_init */
/* TODO: pthread_barrier_wait */
/* TODO: pthread_barrierattr_destroy */
/* TODO: pthread_barrierattr_getpshared */
/* TODO: pthread_barrierattr_init */
/* TODO: pthread_barrierattr_setpshared */
/* TODO: pthread_cancel */
/* TODO: pthread_cleanup_pop */
/* TODO: pthread_cleanup_push */
int pthread_cond_broadcast(pthread_cond_t*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_init(pthread_cond_t* __restrict,
                      const pthread_condattr_t* __restrict);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_timedwait(pthread_cond_t* __restrict,
                           pthread_mutex_t* __restrict,
                           const struct timespec* __restrict);
int pthread_cond_wait(pthread_cond_t* __restrict, pthread_mutex_t* __restrict);
int pthread_condattr_destroy(pthread_condattr_t*);
int pthread_condattr_getclock(const pthread_condattr_t* __restrict,
                              clockid_t* __restrict);
/* TODO: pthread_condattr_getpshared */
int pthread_condattr_init(pthread_condattr_t*);
int pthread_condattr_setclock(pthread_condattr_t*, clockid_t);
/* TODO: pthread_condattr_setpshared */
/* TODO: pthread_create */
/* TODO: pthread_detach */
int pthread_equal(pthread_t, pthread_t);
/* TODO: pthread_exit */
/* TODO: pthread_getconcurrency */
/* TODO: pthread_getcpuclockid */
/* TODO: pthread_getschedparam */
/* TODO: pthread_getspecific */
/* TODO: pthread_join */
/* TODO: pthread_key_create */
/* TODO: pthread_key_delete */
/* TODO: pthread_mutex_consistent */
int pthread_mutex_destroy(pthread_mutex_t*);
/* TODO: pthread_mutex_getprioceiling */
int pthread_mutex_init(pthread_mutex_t* __restrict,
                       const pthread_mutexattr_t* __restrict);
int pthread_mutex_lock(pthread_mutex_t*);
/* TODO: pthread_mutex_setprioceiling */
/* TODO: pthread_mutex_timedlock */
int pthread_mutex_trylock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);
int pthread_mutexattr_destroy(pthread_mutexattr_t*);
/* TODO: pthread_mutexattr_getprioceiling */
/* TODO: pthread_mutexattr_getprotocol */
/* TODO: pthread_mutexattr_getpshared */
/* TODO: pthread_mutexattr_getrobust */
int pthread_mutexattr_gettype(pthread_mutexattr_t* __restrict, int* __restrict);
int pthread_mutexattr_init(pthread_mutexattr_t*);
/* TODO: pthread_mutexattr_setprioceiling */
/* TODO: pthread_mutexattr_setprotocol */
/* TODO: pthread_mutexattr_setpshared */
/* TODO: pthread_mutexattr_setrobust */
int pthread_mutexattr_settype(pthread_mutexattr_t*, int);
/* TODO: pthread_once */
int pthread_rwlock_destroy(pthread_rwlock_t*);
int pthread_rwlock_init(pthread_rwlock_t* __restrict,
                        const pthread_rwlockattr_t* __restrict);
int pthread_rwlock_rdlock(pthread_rwlock_t*);
/* TODO: pthread_rwlock_timedrdlock */
/* TODO: pthread_rwlock_timedwrlock */
int pthread_rwlock_tryrdlock(pthread_rwlock_t*);
int pthread_rwlock_trywrlock(pthread_rwlock_t*);
int pthread_rwlock_unlock(pthread_rwlock_t*);
int pthread_rwlock_wrlock(pthread_rwlock_t*);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t*);
/* TODO: pthread_rwlockattr_getpshared */
int pthread_rwlockattr_init(pthread_rwlockattr_t*);
/* TODO: pthread_rwlockattr_setpshared */
pthread_t pthread_self(void);
/* TODO: pthread_setcancelstate */
/* TODO: pthread_setcanceltype */
/* TODO: pthread_setconcurrency */
/* TODO: pthread_setschedparam */
/* TODO: pthread_setschedprio */
/* TODO: pthread_setspecific */
/* TODO: pthread_spin_destroy */
/* TODO: pthread_spin_init */
/* TODO: pthread_spin_lock */
/* TODO: pthread_spin_trylock */
/* TODO: pthread_spin_unlock */
/* TODO: pthread_testcancel */

__END_DECLS

#endif
