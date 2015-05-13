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

    sortix/resource.h
    Resource limits and operations.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_RESOURCE_H
#define INCLUDE_SORTIX_RESOURCE_H

#include <sys/cdefs.h>

#include <__/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

typedef __uintmax_t rlim_t;

#define RLIM_INFINITY __UINTMAX_MAX
#define RLIM_SAVED_CUR RLIM_INFINITY
#define RLIM_SAVED_MAX RLIM_INFINITY

struct rlimit
{
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

#define RLIMIT_AS 0
#define RLIMIT_CORE 1
#define RLIMIT_CPU 2
#define RLIMIT_DATA 3
#define RLIMIT_FSIZE 4
#define RLIMIT_NOFILE 5
#define RLIMIT_STACK 6
#define __RLIMIT_NUM_DECLARED 7 /* index of highest constant plus 1. */

#if defined(__is_sortix_kernel)
#define RLIMIT_NUM_DECLARED __RLIMIT_NUM_DECLARED
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
