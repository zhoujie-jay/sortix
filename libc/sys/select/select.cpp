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

    sys/select/select.cpp
    Waiting on multiple file descriptors.

*******************************************************************************/

#include <sys/select.h>

#include <poll.h>

extern "C"
int select(int nfds, fd_set* restrict readfds, fd_set* restrict writefds,
           fd_set* restrict exceptfds, struct timeval* restrict timeout)
{
	const int READ_EVENTS = POLLIN | POLLRDNORM;
	const int WRITE_EVENTS = POLLOUT | POLLWRNORM;
	const int EXCEPT_EVENTS = POLLERR | POLLHUP;
	struct pollfd fds[FD_SETSIZE];
	for ( int i = 0; i < nfds; i++ )
	{
		fds[i].fd = i;
		fds[i].events = fds[i].revents = 0;
		if ( FD_ISSET(i, readfds) )
			fds[i].events |= READ_EVENTS;
		if ( FD_ISSET(i, writefds) )
			fds[i].events |= WRITE_EVENTS;
		if ( FD_ISSET(i, exceptfds) )
			fds[i].events |= EXCEPT_EVENTS;
		if ( !fds[i].events )
			fds[i].fd = -1;
	}
	struct timespec* timeout_tsp = NULL;
	struct timespec timeout_ts;
	if ( timeout )
		timeout_tsp = &timeout_ts,
		timeout_tsp->tv_sec = timeout->tv_sec,
		timeout_tsp->tv_nsec = (long) timeout->tv_usec * 1000;
	int num_occur = ppoll(fds, nfds, timeout_tsp, NULL);
	if ( num_occur < 0 )
		return -1;
	if ( readfds ) FD_ZERO(readfds);
	if ( writefds ) FD_ZERO(writefds);
	if ( exceptfds ) FD_ZERO(exceptfds);
	int ret = 0;
	for ( int i = 0; i < nfds; i++ )
	{
		if ( !fds[i].events )
			continue;
		int events = fds[i].revents;
		if ( events & READ_EVENTS && readfds ) { FD_SET(i, readfds); ret++; }
		if ( events & WRITE_EVENTS && writefds ) { FD_SET(i, writefds); ret++; }
		if ( events & EXCEPT_EVENTS && exceptfds ) { FD_SET(i, exceptfds); ret++; }
	}
	return ret;
}
