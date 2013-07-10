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

    write.cpp
    Writes to a file descriptor.

*******************************************************************************/

#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>

DEFN_SYSCALL3(ssize_t, SysWrite, SYSCALL_WRITE, int, const void*, size_t);

extern "C" ssize_t write(int fd, const void* buf, size_t count)
{
retry:
	ssize_t result = SysWrite(fd, buf, count);
	if ( result < 0 && errno == EAGAIN ) { goto retry; }
	return result;
}

DEFN_SYSCALL4(ssize_t, sys_pwrite, SYSCALL_PWRITE, int, const void*, size_t, off_t);

extern "C" ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset)
{
	return sys_pwrite(fd, buf, count, offset);
}
