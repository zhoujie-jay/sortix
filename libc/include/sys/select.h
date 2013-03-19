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

    sys/select.h
    Waiting on multiple file descriptors.

*******************************************************************************/


#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H 1

#include <features.h>
#include <string.h> /* TODO: HACK: for FD_ZERO */

__BEGIN_DECLS

@include(time_t.h)
@include(suseconds_t.h)

__BEGIN_DECLS
#include <sortix/sigset.h>
#include <sortix/timespec.h>
#include <sortix/timeval.h>
__END_DECLS

#define FD_SETSIZE 1024
#define __FD_ELEM_SIZE ((int) sizeof(__fd_mask))
#define __FD_ELEM_BITS (8 * __FD_ELEM_SIZE)
typedef long int __fd_mask;
typedef struct
{
	__fd_mask __fds_bits[FD_SETSIZE / (8 * (int) sizeof(__fd_mask))];
} fd_set;

#define __FD_INDEX(fd) ((fd) / __FD_ELEM_BITS)
#define __FD_EXPONENT(fd) ((__fd_mask) (fd) % __FD_ELEM_BITS)
#define __FD_MASK(fd) (1 << __FD_EXPONENT(fd))
#define __FD_ACCESS(fd, fdsetp) ((fdsetp)->__fds_bits[__FD_INDEX(fd)])

#define FD_CLR(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) &= ~__FD_MASK(fd))
#define FD_ISSET(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) & __FD_MASK(fd))
#define FD_SET(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) |= __FD_MASK(fd))
#define FD_ZERO(fdsetp) memset(fdsetp, 0, sizeof(fd_set))

/* TODO: pselect */
int select(int, fd_set* restrict, fd_set* restrict, fd_set* restrict,
           struct timeval* restrict);

__END_DECLS

#endif