/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	sortix/fork.h
	Declarations related to the fork family of system calls on Sortix.

*******************************************************************************/

#ifndef SORTIX_FORK_H
#define SORTIX_FORK_H

#include <features.h>
#include <sortix/x86/fork.h>
#include <sortix/x64/fork.h>

__BEGIN_DECLS

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

/* The sfork system call is much like the rfork system call found in Plan 9 and
   BSD systems, however it works slightly differently and was renamed to avoid
   conflicts with existing programs. In particular, it never forks an item
   unless its bit is set, whereas rfork sometimes forks an item by default. If
   you wish to fork certain items simply set the proper flags. Note that since
   flags may be added from time to time, you should use various compound flags
   defined below such as SFFORK and SFALL. It can be useful do combine these
   compount flags with bitoperations, for instance "I want traditional fork,
   except share the working dir pointer" is sfork(SFFORK & ~SFCWD). */
#define SFPROC (1<<0) /* Creates child, otherwise affect current process. */
#define SFPID (1<<1) /* Allocates new PID. */
#define SFFD (1<<2) /* Fork file descriptor table. */
#define SFMEM (1<<3) /* Forks address space. */
#define SFCWD (1<<4) /* Forks current directory pointer. */
#define SFROOT (1<<5) /* Forks root directory pointer. */
#define SFNAME (1<<6) /* Forks namespace. */
#define SFSIG (1<<7) /* Forks signal table. */
#define SFCSIG (1<<8) /* Child will have no pending signals, like fork(2). */

/* Creates a new thread in this process. Beware that it will share the stack of
   the parent thread and that various threading featues may not have been set up
   properly. You should use the standard threading API unless you know what you
   are doing; remember that you can always sfork more stuff after the standard
   threading API returns control to you. */
#define SFTHREAD (SFPROC | SFCSIG)

/* Provides traditional fork(2) behavior; use this instead of the above values
   if you want "as fork(2), but also fork foo", or "as fork(2), except bar". In
   those cases it is better to sfork(SFFORK & ~SFFOO); or sfork(SFFORK | SFBAR);
   as that would allow to add new flags to SFFORK if a new kernel feature is
   added to the system that applications don't know about yet. */
#define SFFORK (SFPROC | SFPID | SFFD | SFMEM | SFCWD | SFROOT | SFCSIG)

/* This allows creating a process that is completely forked from the original
   process, unlike SFFORK which does share a few things (such as the process
   namespace). Note that there is a few unset high bits in this value, these
   are reserved and must not be set. */
#define SFALL ((1<<20)-1)

#ifdef PLATFORM_X86
typedef struct sforkregs_x86 sforkregs_t;
#elif defined(PLATFORM_X64)
typedef struct sforkregs_x64 sforkregs_t;
#else
#warning No sforkresgs_cpu structure declared
#endif

__END_DECLS

#endif

