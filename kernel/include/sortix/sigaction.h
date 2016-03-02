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
 * sortix/sigaction.h
 * Declares struct sigaction and associated flags.
 */

#ifndef INCLUDE_SORTIX_SIGACTION_H
#define INCLUDE_SORTIX_SIGACTION_H

#include <sys/cdefs.h>

#include <sortix/siginfo.h>
#include <sortix/sigset.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SA_NOCLDSTOP (1<<0)
#define SA_ONSTACK (1<<1)
#define SA_RESETHAND (1<<2)
#define SA_RESTART (1<<3)
#define SA_SIGINFO (1<<4)
#define SA_NOCLDWAIT (1<<5)
#define SA_NODEFER (1<<6)
#define SA_COOKIE (1<<7)

#define __SA_SUPPORTED_FLAGS (SA_NOCLDSTOP | SA_ONSTACK | SA_RESETHAND | \
                              SA_RESTART | SA_SIGINFO | SA_NOCLDWAIT | \
                              SA_NODEFER | SA_COOKIE)

struct sigaction
{
	sigset_t sa_mask;
	__extension__ union
	{
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t*, void*);
		void (*sa_sigaction_cookie)(int, siginfo_t*, void*, void*);
	};
	void* sa_cookie;
	int sa_flags;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
