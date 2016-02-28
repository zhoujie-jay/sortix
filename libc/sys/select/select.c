/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2016.

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

    sys/select/select.c
    Waiting on multiple file descriptors.

*******************************************************************************/

#include <sys/select.h>

#include <errno.h>
#include <poll.h>

static const int READ_EVENTS = POLLIN | POLLRDNORM;
static const int WRITE_EVENTS = POLLOUT | POLLWRNORM;
static const int EXCEPT_EVENTS = POLLERR | POLLHUP;

int select(int nfds, fd_set* restrict readfds, fd_set* restrict writefds,
           fd_set* restrict exceptfds, struct timeval* restrict timeout)
{
	if ( nfds < 0 || FD_SETSIZE < nfds )
		return errno = EINVAL, -1;
	if ( timeout )
	{
		if ( timeout->tv_sec < 0 )
			return errno = EINVAL, -1;
		if ( timeout->tv_usec < 0 || 1000000 <= timeout->tv_usec )
			return errno = EINVAL, -1;
	}
	struct pollfd fds[FD_SETSIZE];
	nfds_t fds_count = 0;
	for ( int i = 0; i < nfds; i++ )
	{
		fds[fds_count].fd = i;
		fds[fds_count].events = fds[fds_count].revents = 0;
		if ( readfds && FD_ISSET(i, readfds) )
		{
			FD_CLR(i, readfds);
			fds[fds_count].events |= READ_EVENTS;
		}
		if ( writefds && FD_ISSET(i, writefds) )
		{
			FD_CLR(i, writefds);
			fds[fds_count].events |= WRITE_EVENTS;
		}
		if ( exceptfds && FD_ISSET(i, exceptfds) )
		{
			FD_CLR(i, exceptfds);
			fds[fds_count].events |= EXCEPT_EVENTS;
		}
		if ( fds[fds_count].events )
			fds_count++;
	}
	struct timespec* timeout_tsp = NULL;
	struct timespec timeout_ts;
	if ( timeout )
	{
		timeout_tsp = &timeout_ts;
		timeout_tsp->tv_sec = timeout->tv_sec;
		timeout_tsp->tv_nsec = (long) timeout->tv_usec * 1000L;
	}
	int num_occur = ppoll(fds, fds_count, timeout_tsp, NULL);
	if ( num_occur < 0 )
		return -1;
	int ret = 0;
	for ( nfds_t i = 0; i < fds_count; i++ )
	{
		int fd = fds[i].fd;
		int events = fds[i].revents;
		if ( events & READ_EVENTS && readfds )
		{
			FD_SET(fd, readfds);
			ret++;
		}
		if ( events & WRITE_EVENTS && writefds )
		{
			FD_SET(fd, writefds);
			ret++;
		}
		if ( events & EXCEPT_EVENTS && exceptfds )
		{
			FD_SET(fd, exceptfds);
			ret++;
		}
	}
	return ret;
}
