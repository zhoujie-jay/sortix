/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    poll/ppoll.cpp
    Input/output multiplexing.

*******************************************************************************/

#include <sys/syscall.h>

#include <poll.h>

DEFN_SYSCALL4(int, sys_ppoll, SYSCALL_PPOLL, struct pollfd*, nfds_t,
              const struct timespec*, const sigset_t*);

extern "C" int ppoll(struct pollfd* fds, nfds_t nfds,
                     const struct timespec* timeout, const sigset_t* sigmask)
{
	return sys_ppoll(fds, nfds, timeout, sigmask);
}
