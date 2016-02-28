/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sys/resource/prlimit.c
    Access and modify the resource limits of the given process.

*******************************************************************************/

#include <sys/resource.h>
#include <sys/syscall.h>

DEFN_SYSCALL4(int, sys_prlimit, SYSCALL_PRLIMIT,
              pid_t,
              int,
              const struct rlimit*,
              struct rlimit*);

int prlimit(pid_t pid,
            int resource,
            const struct rlimit* new_limit,
            struct rlimit* old_limit)
{
	return sys_prlimit(pid, resource, new_limit, old_limit);
}
