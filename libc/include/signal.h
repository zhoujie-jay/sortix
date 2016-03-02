/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * signal.h
 * Signal API.
 */

#ifndef INCLUDE_SIGNAL_H
#define INCLUDE_SIGNAL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if __STDC_HOSTED__
#include <__/pthread.h>
#endif

#include <sortix/sigaction.h>
#include <sortix/sigevent.h>
#include <sortix/siginfo.h>
#include <sortix/signal.h>
#include <sortix/signal.h>
#include <sortix/sigprocmask.h>
#include <sortix/sigset.h>
#include <sortix/sigval.h>
#include <sortix/stack.h>
#include <sortix/timespec.h>
#include <sortix/ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
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

#if __STDC_HOSTED__

#ifndef __pthread_attr_t_defined
#define __pthread_attr_t_defined
typedef __pthread_attr_t pthread_attr_t;
#endif

#ifndef __pthread_t_defined
#define __pthread_t_defined
typedef __pthread_t pthread_t;
#endif

#endif

#define NSIG __SIG_MAX_NUM

typedef int sig_atomic_t;

#define MINSIGSTKSZ 2048
#define SIGSTKSZ 8192

int kill(pid_t, int);
int killpg(pid_t, int);
void psiginfo(const siginfo_t*, const char*);
void psignal(int, const char*);
/* TODO: int pthread_kill(pthread_t, int); */
int pthread_sigmask(int, const sigset_t* __restrict, sigset_t* __restrict);
int raise(int sig);
int sigaction(int, const struct sigaction* __restrict, struct sigaction* __restrict);
int sigaddset(sigset_t*, int);
int sigaltstack(const stack_t* __restrict, stack_t* __restrict);
int sigandset(sigset_t*, const sigset_t*, const sigset_t*);
int sigdelset(sigset_t*, int);
int sigemptyset(sigset_t*);
int sigfillset(sigset_t*);
int sigisemptyset(const sigset_t*);
int sigismember(const sigset_t*, int);
void (*signal(int, void (*)(int)))(int);
int signotset(sigset_t*, const sigset_t*);
int sigorset(sigset_t*, const sigset_t*, const sigset_t*);
int sigpending(sigset_t*);
int sigprocmask(int, const sigset_t* __restrict, sigset_t* __restrict);
/* TODO: int sigqueue(pid_t, int, const union sigval); */
int sigsuspend(const sigset_t*);
/* TODO: int sigtimedwait(const sigset_t* __restrict, siginfo_t* __restrict,
                          const struct timespec* __restrict); */
/* TODO: int sigwait(const sigset_t* __restrict, int* __restrict); */
/* TODO: int sigwaitinfo(const sigset_t* __restrict, siginfo_t* __restrict); */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
