/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    sortix/psctl.h
    Process control interface.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_PSCTL_H
#define INCLUDE_SORTIX_PSCTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/tmns.h>

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

#define __PSCTL(s, v) (sizeof(struct s) << 16 | (v))

#define PSCTL_PREV_PID __PSCTL(psctl_prev_pid, 1)
struct psctl_prev_pid
{
	pid_t prev_pid;
};

#define PSCTL_NEXT_PID __PSCTL(psctl_next_pid, 2)
struct psctl_next_pid
{
	pid_t next_pid;
};

#define PSCTL_STAT __PSCTL(psctl_stat, 3)
struct psctl_stat
{
	pid_t pid;
	pid_t ppid;
	pid_t ppid_prev;
	pid_t ppid_next;
	pid_t ppid_first;
	pid_t pgid;
	pid_t pgid_prev;
	pid_t pgid_next;
	pid_t pgid_first;
	pid_t sid;
	pid_t sid_prev;
	pid_t sid_next;
	pid_t sid_first;
	pid_t init;
	pid_t init_prev;
	pid_t init_next;
	pid_t init_first;
	uid_t uid;
	uid_t euid;
	gid_t gid;
	gid_t egid;
	int status;
	int nice;
	struct tmns tmns;
};

#define PSCTL_PROGRAM_PATH __PSCTL(psctl_program_path, 4)
struct psctl_program_path
{
	char* buffer;
	size_t size;
};

#endif
