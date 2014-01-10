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

    signal.h
    Signals.

*******************************************************************************/

#ifndef INCLUDE_SIGNAL_H
#define INCLUDE_SIGNAL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if __STDC_HOSTED__
#include <__/pthread.h>
#endif

#include <sortix/signal.h>

__BEGIN_DECLS

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

/* TODO: POSIX says this header declares struct timespec, but not time_t... */
#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
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

__END_DECLS

#include <sortix/sigset.h>
#include <sortix/timespec.h>

__BEGIN_DECLS

/* TODO: Should this be volatile? It isn't on Linux. */
typedef int sig_atomic_t;

typedef void (*sighandler_t)(int);

void SIG_DFL(int);
void SIG_IGN(int);
void SIG_ERR(int);

#define SIG_DFL SIG_DFL
#define SIG_IGN SIG_IGN
#define SIG_ERR SIG_ERR
/* TODO: POSIX specifies a obsolecent SIG_HOLD here. */

union sigval
{
	int sival_int;
	void* sival_ptr;
};

struct sigevent
{
	int sigev_notify;
	int sigev_signo;
	union sigval sigev_value;
	void (*sigev_notify_function)(union sigval);
	/*pthread_attr_t* sigev_notify_attributes;*/
};

#define SIGEV_NONE 0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

/* TODO: SIGRTMIN */
/* TODO: SIGRTMAX */

typedef struct
{
	int si_signo;
	int si_code;
	int si_errno;
	pid_t si_pid;
	uid_t si_uid;
	void* si_addr;
	int si_status;
	union sigval si_value;
} siginfo_t;

#define ILL_ILLOPC 1
#define ILL_ILLOPN 2
#define ILL_ILLADR 3
#define ILL_ILLTRP 4
#define ILL_PRVOPC 5
#define ILL_PRVREG 6
#define ILL_COPROC 7
#define ILL_BADSTK 8
#define FPE_INTDIV 9
#define FPE_INTOVF 10
#define FPE_FLTDIV 11
#define FPE_FLTOVF 12
#define FPE_FLTUND 13
#define FPE_FLTRES 14
#define FPE_FLTINV 15
#define FPE_FLTSUB 16
#define SEGV_MAPERR 17
#define SEGV_ACCERR 18
#define BUS_ADRALN 19
#define BUS_ADRERR 20
#define BUS_OBJERR 21
#define TRAP_BRKPT 22
#define TRAP_TRACE 23
#define CLD_EXITED 24
#define CLD_KILLED 25
#define CLD_DUMPED 26
#define CLD_TRAPPED 27
#define CLD_STOPPED 29
#define CLD_CONTINUED 30
#define SI_USER 31
#define SI_QUEUE 32
#define SI_TIMER 33
#define SI_ASYNCIO 34
#define SI_MSGQ 35

struct sigaction
{
	void (*sa_handler)(int);
	void (*sa_sigaction)(int, siginfo_t*, void*);
	sigset_t sa_mask;
	int sa_flags;
};

#define SA_NOCLDSTOP (1<<0)
#define SA_ONSTACK (1<<1)
#define SA_RESETHAND (1<<2)
#define SA_RESTART (1<<3)
#define SA_SIGINFO (1<<4)
#define SA_NOCLDWAIT (1<<5)
#define SA_NODEFER (1<<6)
#define SS_ONSTACK (1<<7)
#define SS_DISABLE (1<<8)
/* TODO: MINSIGSTKSZ */
/* TODO: SIGSTKSZ */

/* TODO: mcontext_t */
typedef int mcontext_t;

typedef struct
{
	void* ss_sp;
	size_t ss_size;
	int ss_flags;
} stack_t;

typedef struct __ucontext ucontext_t;
struct __ucontext
{
	ucontext_t* uc_link;
	sigset_t uc_sigmask;
	stack_t uc_stack;
	mcontext_t uc_mcontext;
};

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
/* TODO: sighold (obsolescent XSI). */
/* TODO: sigignore (obsolescent XSI). */
/* TODO: siginterrupt (obsolescent XSI). */
int sigisemptyset(const sigset_t*);
int sigismember(const sigset_t*, int);
sighandler_t signal(int, sighandler_t);
int signotset(sigset_t*, const sigset_t*);
int sigorset(sigset_t*, const sigset_t*, const sigset_t*);
/* TODO: sigpause (obsolescent XSI). */
int sigpending(sigset_t*);
int sigprocmask(int, const sigset_t* __restrict, sigset_t* __restrict);
int sigqueue(pid_t, int, const union sigval);
/* TODO: sigrelse (obsolescent XSI). */
/* TODO: sigset (obsolescent XSI). */
int sigsuspend(const sigset_t*);
int sigtimedwait(const sigset_t* __restrict, siginfo_t* __restrict,
                 const struct timespec* __restrict);
int sigwait(const sigset_t* __restrict, int* __restrict);
int sigwaitinfo(const sigset_t* __restrict, siginfo_t* vrestrict);

__END_DECLS

#endif
