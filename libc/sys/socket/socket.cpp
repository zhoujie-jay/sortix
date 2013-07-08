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

    sys/socket/socket.cpp
    Creates a new unconnected socket.

*******************************************************************************/

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

extern "C" int socket(int domain, int type, int protocol)
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
