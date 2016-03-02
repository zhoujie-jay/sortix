/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sortix/resource.h
 * Resource limits and operations.
 */

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
