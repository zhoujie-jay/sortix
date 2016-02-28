/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    sys/socket/sendto.c
    Send a message on a socket.

*******************************************************************************/

#include <sys/socket.h>

#include <string.h>

ssize_t sendto(int fd,
               const void* buffer,
               size_t buffer_size,
               int flags,
               const struct sockaddr* addr,
               socklen_t addrsize)
{
	struct msghdr msghdr;
	memset(&msghdr, 0, sizeof(msghdr));

	struct iovec iovec;
	iovec.iov_base = (void*) buffer;
	iovec.iov_len = buffer_size;

	msghdr.msg_name = (void*) addr;
	msghdr.msg_namelen = addrsize;
	msghdr.msg_iov = &iovec;
	msghdr.msg_iovlen = 1;

	return sendmsg(fd, &msghdr, flags);
}
