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

    sys/socket/send.cpp
    Receive data on a socket.

*******************************************************************************/

#include <sys/socket.h>
#include <sys/syscall.h>

DEFN_SYSCALL4(ssize_t, sys_send, SYSCALL_SEND, int, const void*, size_t, int);

extern "C" ssize_t send(int fd, const void* buf, size_t count, int flags)
{
	return sys_send(fd, buf, count, flags);
}
