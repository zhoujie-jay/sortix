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
 * sortix/siginfo.h
 * Declares siginfo_t and associated flags.
 */

#ifndef INCLUDE_SORTIX_SIGINFO_H
#define INCLUDE_SORTIX_SIGINFO_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/sigval.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

typedef struct
{
	union sigval si_value;
	void* si_addr;
	pid_t si_pid;
	uid_t si_uid;
	int si_signo;
	int si_code;
	int si_errno;
	int si_status;
} siginfo_t;

/* TODO: Decide appropriate values for these constants: */
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
