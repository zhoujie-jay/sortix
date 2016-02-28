/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    fsmarshall/fsm_fsbind.c
    Binds a user-space filesystem inode at a mount point.

*******************************************************************************/

#include <sys/syscall.h>

#include <errno.h>

#include <fsmarshall.h>

DEFN_SYSCALL3(int, sys_fsm_fsbind, SYSCALL_FSM_FSBIND, int, int, int);

int fsm_fsbind(int rootfd, int mountpoint, int flags)
{
	return sys_fsm_fsbind(rootfd, mountpoint, flags);
}
