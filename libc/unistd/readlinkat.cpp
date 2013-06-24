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

    unistd/readlinkat.cpp
    Read contents of symbolic link.

*******************************************************************************/

#include <sys/syscall.h>

#include <fcntl.h>
#include <unistd.h>

DEFN_SYSCALL4(ssize_t, sys_readlinkat, SYSCALL_READLINKAT, int,
              const char* restrict, char* restrict, size_t);

extern "C" ssize_t readlinkat(int dirfd, const char* restrict path,
                              char* restrict buf, size_t size)
{
	return sys_readlinkat(dirfd, path, buf, size);
}
