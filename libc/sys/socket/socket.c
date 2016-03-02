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
 * sys/socket/socket.c
 * Creates a new unconnected socket.
 */

#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>

static const char* get_socket_factory(int domain, int type, int protocol)
{
	if ( domain == AF_INET )
	{
		if ( type == SOCK_DGRAM && !protocol )
			return "/dev/net/ipv4/udp";
		if ( type == SOCK_STREAM && !protocol )
			return "/dev/net/ipv4/tcp";
		return errno = EPROTONOSUPPORT, (const char*) NULL;
	}

	if ( domain == AF_INET6 )
	{
		if ( type == SOCK_DGRAM && !protocol )
			return "/dev/net/ipv6/udp";
		if ( type == SOCK_STREAM && !protocol )
			return "/dev/net/ipv6/tcp";
		return errno = EPROTONOSUPPORT, (const char*) NULL;
	}

	if ( domain == AF_UNIX )
	{
		if ( type == SOCK_DGRAM && !protocol )
			return "/dev/net/fs/datagram";
		if ( type == SOCK_STREAM && !protocol )
			return "/dev/net/fs/stream";
		return errno = EPROTONOSUPPORT, (const char*) NULL;
	}

	return errno = EAFNOSUPPORT, (const char*) NULL;
}

int socket(int domain, int type, int protocol)
{
	int open_flags = O_RDWR;
	if ( type & SOCK_NONBLOCK ) open_flags |= O_NONBLOCK;
	if ( type & SOCK_CLOEXEC ) open_flags |= O_CLOEXEC;
	if ( type & SOCK_CLOFORK ) open_flags |= O_CLOFORK;
	type &= SOCK_TYPE_MASK;
	const char* factory = get_socket_factory(domain, type, protocol);
	if ( !factory )
		return -1;
	int ret = open(factory, open_flags);
	if ( ret < 0 && errno == ENOENT )
		errno = EPROTONOSUPPORT;
	return ret;
}
