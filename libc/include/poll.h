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

    poll.h
    Input/output multiplexing.

*******************************************************************************/

#ifndef _POLL_H
#define _POLL_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#include <sortix/poll.h>
#include <sortix/sigset.h>
#include <sortix/timespec.h>
#ifdef __cplusplus
extern "C" {
#endif

int poll(struct pollfd* fds, nfds_t nfds, int timeout);
int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* timeout,
          const sigset_t* sigmask);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
