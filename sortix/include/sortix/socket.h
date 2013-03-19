/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/socket.h
    Declarations for socket types and other flags.

*******************************************************************************/

#ifndef SORTIX_INCLUDE_SOCKET_H
#define SORTIX_INCLUDE_SOCKET_H

#include <features.h>

__BEGIN_DECLS

/* TODO: Nicely wrap this in an enum, as in glibc's header? */
#define SOCK_TYPE_MASK ((1<<20)-1)
#define SOCK_RAW 0 /* Will Sortix support this? */
#define SOCK_DGRAM 1
#define SOCK_SEQPACKET 2
#define SOCK_STREAM 3

#define SOCK_NONBLOCK (1<<20)
#define SOCK_CLOEXEC (1<<21)
#define SOCK_CLOFORK (1<<22)

__END_DECLS

#endif
