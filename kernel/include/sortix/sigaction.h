/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    sortix/sigaction.h
    Declares struct sigaction and associated flags.

*******************************************************************************/

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
