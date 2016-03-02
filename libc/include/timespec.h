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
 * timespec.h
 * Utility functions for manipulation of struct timespec.
 */

#ifndef INCLUDE_TIMESPEC_H
#define INCLUDE_TIMESPEC_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

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

#include <sortix/timespec.h>

#ifdef __cplusplus
extern "C" {
#endif

static __inline bool timespec_eq(struct timespec a, struct timespec b)
{
	return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

static __inline bool timespec_neq(struct timespec a, struct timespec b)
{
	return a.tv_sec != b.tv_sec || a.tv_nsec != b.tv_nsec;
}

static __inline bool timespec_lt(struct timespec a, struct timespec b)
{
	return a.tv_sec < b.tv_sec ||
	       (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec);
}

static __inline bool timespec_le(struct timespec a, struct timespec b)
{
	return a.tv_sec < b.tv_sec ||
	       (a.tv_sec == b.tv_sec && a.tv_nsec <= b.tv_nsec);
}

static __inline bool timespec_gt(struct timespec a, struct timespec b)
{
	return a.tv_sec > b.tv_sec ||
	       (a.tv_sec == b.tv_sec && a.tv_nsec > b.tv_nsec);
}

static __inline bool timespec_ge(struct timespec a, struct timespec b)
{
	return a.tv_sec > b.tv_sec ||
	       (a.tv_sec == b.tv_sec && a.tv_nsec >= b.tv_nsec);
}

static __inline struct timespec timespec_make(time_t sec, long nsec)
{
	struct timespec ret;
	ret.tv_sec = sec;
	ret.tv_nsec = nsec;
	return ret;
}

static __inline struct timespec timespec_neg(struct timespec t)
{
	if ( t.tv_nsec )
		return timespec_make(-t.tv_sec - 1, 1000000000 - t.tv_nsec);
	return timespec_make(-t.tv_sec, 0);
}

static __inline struct timespec timespec_nul(void)
{
	return timespec_make(0, 0);
}

struct timespec timespec_canonalize(struct timespec t);
struct timespec timespec_add(struct timespec a, struct timespec b);
struct timespec timespec_sub(struct timespec a, struct timespec b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
