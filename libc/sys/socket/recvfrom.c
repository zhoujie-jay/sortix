/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sys/socket/recvfrom.c
 * Receive a message from a socket.
 */

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
