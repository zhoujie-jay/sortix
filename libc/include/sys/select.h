/*
 * Copyright (c) 2013, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sys/select.h
 * Waiting on multiple file descriptors.
 */


#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <string.h> /* TODO: HACK: for FD_ZERO */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifndef __suseconds_t_defined
#define __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#endif

#ifndef __timeval_defined
#define __timeval_defined
struct timeval
{
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <sortix/sigset.h>
#include <sortix/timespec.h>
#ifdef __cplusplus
} /* extern "C" */
#endif

#define FD_SETSIZE 1024
#define __FD_ELEM_SIZE ((int) sizeof(__fd_mask))
#define __FD_ELEM_BITS (8 * __FD_ELEM_SIZE)
typedef unsigned long __fd_mask;
typedef struct
{
	__fd_mask __fds_bits[FD_SETSIZE / (8 * (int) sizeof(__fd_mask))];
} fd_set;

#define __FD_INDEX(fd) ((fd) / __FD_ELEM_BITS)
#define __FD_EXPONENT(fd) ((__fd_mask) (fd) % __FD_ELEM_BITS)
#define __FD_MASK(fd) (1UL << __FD_EXPONENT(fd))
#define __FD_ACCESS(fd, fdsetp) ((fdsetp)->__fds_bits[__FD_INDEX(fd)])

#define FD_CLR(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) &= ~__FD_MASK(fd))
#define FD_ISSET(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) & __FD_MASK(fd))
#define FD_SET(fd, fdsetp) (__FD_ACCESS(fd, fdsetp) |= __FD_MASK(fd))
#define FD_ZERO(fdsetp) memset(fdsetp, 0, sizeof(fd_set))

/* TODO: pselect */
int select(int, fd_set* __restrict, fd_set* __restrict, fd_set* __restrict,
           struct timeval* __restrict);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
