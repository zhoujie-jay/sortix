/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    sys/wait.h
    Declarations for waiting.

*******************************************************************************/

// TODO: Make this header comply with POSIX-1.2008

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/wait.h>

__BEGIN_DECLS

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

/* TODO: These are not implemented in sortix libc yet. */
#if 0
int waitid(idtype_t, id_t, siginfo_t*, int);
#endif

pid_t wait(int* stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

__END_DECLS

#endif
