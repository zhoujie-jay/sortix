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

    unistd/fchownat.cpp
    Changes the owner and group of a file.

*******************************************************************************/

#include <sys/types.h>
#include <sys/syscall.h>

#include <fcntl.h>
#include <unistd.h>

DEFN_SYSCALL5(int, sys_fchownat, SYSCALL_FCHOWNAT, int, const char*, uid_t, gid_t, int);

extern "C" int fchownat(int dirfd, const char* path, uid_t owner, gid_t group, int flags)
{
	return sys_fchownat(dirfd, path, owner, group, flags);
}
