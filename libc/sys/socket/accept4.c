/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sys/socket/accept4.c
 * Accepts a connection from a listening socket.
 */

#include <sys/socket.h>
#include <sys/syscall.h>

#include <stddef.h>

DEFN_SYSCALL4(int, sys_accept4, SYSCALL_ACCEPT4, int, void*, size_t*, int);

int accept4(int fd, struct sockaddr* restrict addr, socklen_t* restrict addrlen,
            int flags)
{
	// Deal with that the kernel doesn't need to understand socklen_t. Since
	// that type is designed to be size_t-sized anyway, the compiler should be
	// able to optimize all this type safety away, but this function will work
	// should the assumption sizeof(size_t) == sizeof(socklen_t) change.
	size_t addrlen_size_t = addrlen ? *addrlen : 0;
	size_t* addrlen_size_t_ptr = addrlen ? &addrlen_size_t : NULL;
	int retfd = sys_accept4(fd, addr, addrlen_size_t_ptr, flags);
	if ( addrlen )
		*addrlen = addrlen_size_t;
	return retfd;
}
