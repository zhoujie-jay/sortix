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

    sys/socket/accept4.cpp
    Accepts a connection from a listening socket.

*******************************************************************************/

#include <sys/socket.h>
#include <sys/syscall.h>

#include <stddef.h>

DEFN_SYSCALL4(int, sys_accept4, SYSCALL_ACCEPT4, int, void*, size_t*, int);

extern "C" int accept4(int fd, struct sockaddr* restrict addr,
                       socklen_t* restrict addrlen, int flags)
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
