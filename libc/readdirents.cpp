/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    readdirents.cpp
    Reads entries from a directory file descriptor.

*******************************************************************************/

#include <sys/readdirents.h>
#include <sys/syscall.h>

DEFN_SYSCALL4(ssize_t, sys_readdirents, SYSCALL_READDIRENTS, int, struct kernel_dirent*, size_t, size_t);

extern "C" ssize_t readdirents(int fd, struct kernel_dirent* dirent, size_t size)
{
	return sys_readdirents(fd, dirent, size, 1);
}
