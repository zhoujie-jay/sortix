/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    fsmarshall/fsm_mountat.cpp
    Attaches a user-space filesystem at the specified mount point.

*******************************************************************************/

#include <sys/syscall.h>

#include <fsmarshall.h>

DEFN_SYSCALL4(int, sys_fsm_mountat, SYSCALL_FSM_MOUNTAT, int, const char*, const struct stat*, int);

extern "C" int fsm_mountat(int dirfd, const char* path, const struct stat* rootst, int flags)
{
	return sys_fsm_mountat(dirfd, path, rootst, flags);
}