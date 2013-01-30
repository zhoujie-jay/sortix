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

    fsmarshall.h
    User-space filesystem API.

*******************************************************************************/

#ifndef INCLUDE_FSMARSHALL_H
#define INCLUDE_FSMARSHALL_H

#include <stddef.h>

#include <sortix/stat.h>
#include <sortix/termios.h>
#include <sortix/timeval.h>

#include <fsmarshall-msg.h>

#if defined(__cplusplus)
extern "C" {
#endif

int fsm_mkserver();
int fsm_closeserver(int server);
int fsm_bootstraprootfd(int server, ino_t ino, int open_flags, mode_t mode);
int fsm_fsbind(int rootfd, int mountpoint, int flags);
int fsm_listen(int server);
int fsm_closechannel(int server, int channel);
ssize_t fsm_recv(int server, int channel, void* ptr, size_t count);
ssize_t fsm_send(int server, int channel, const void* ptr, size_t count);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
