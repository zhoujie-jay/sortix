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

    sys/uio/pwritev.cpp
    Write data from multiple buffers.

*******************************************************************************/

#include <sys/syscall.h>
#include <sys/uio.h>

DEFN_SYSCALL4(ssize_t, sys_pwritev, SYSCALL_PWRITEV, int, const struct iovec*, int, off_t);

extern "C"
ssize_t pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset)
{
	return sys_pwritev(fd, iov, iovcnt, offset);
}
