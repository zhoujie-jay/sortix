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

    writev.cpp
    Write data from multiple buffers.

*******************************************************************************/

#include <sys/syscall.h>
#include <sys/uio.h>

DEFN_SYSCALL3(ssize_t, sys_writev, SYSCALL_WRITEV, int, const struct iovec*, int);

extern "C" ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
{
	return sys_writev(fd, iov, iovcnt);
}
