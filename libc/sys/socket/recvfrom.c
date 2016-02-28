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

    sys/socket/recvfrom.c
    Receive a message from a socket.

*******************************************************************************/

#include <sys/socket.h>

#include <string.h>

ssize_t recvfrom(int fd,
                 void* restrict buffer,
                 size_t buffer_size,
                 int flags,
                 struct sockaddr* restrict addr,
                 socklen_t* restrict addrsize)
{
	struct msghdr msghdr;
	memset(&msghdr, 0, sizeof(msghdr));

	struct iovec iovec;
	iovec.iov_base = buffer;
	iovec.iov_len = buffer_size;

	msghdr.msg_name = addr;
	msghdr.msg_namelen = addrsize ? *addrsize : 0;
	msghdr.msg_iov = &iovec;
	msghdr.msg_iovlen = 1;

	ssize_t result = recvmsg(fd, &msghdr, flags);

	if ( addrsize )
		*addrsize = msghdr.msg_namelen;

	return result;
}
