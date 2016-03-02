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
 * sys/socket/sendto.c
 * Send a message on a socket.
 */

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
