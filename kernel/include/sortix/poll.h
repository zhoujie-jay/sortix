/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2015.

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

    sortix/poll.h
    Interface for waiting on file descriptor events.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_POLL_H
#define INCLUDE_SORTIX_POLL_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef __SIZE_TYPE__ nfds_t;

struct pollfd
{
	int fd;
	short events;
	short revents;
};

#define POLLERR (1<<0)
#define POLLHUP (1<<1)
#define POLLNVAL (1<<2)

#define POLLIN (1<<3)
#define POLLRDNORM (1<<4)
#define POLLRDBAND (1<<5)
#define POLLPRI (1<<6)
#define POLLOUT (1<<7)
#define POLLWRNORM (1<<8)
#define POLLWRBAND (1<<9)

#define POLL__ONLY_REVENTS (POLLERR | POLLHUP | POLLNVAL)

__END_DECLS

#endif
