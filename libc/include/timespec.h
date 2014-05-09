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

    timespec.h
    Utility functions for manipulation of struct timespec.

*******************************************************************************/

#ifndef INCLUDE_TIMESPEC_H
#define INCLUDE_TIMESPEC_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <stdbool.h>

__BEGIN_DECLS

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

__END_DECLS

#include <sortix/timespec.h>

__BEGIN_DECLS

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

static __inline struct timespec timespec_nul()
{
	return timespec_make(0, 0);
}

struct timespec timespec_canonalize(struct timespec t);
struct timespec timespec_add(struct timespec a, struct timespec b);
struct timespec timespec_sub(struct timespec a, struct timespec b);

__END_DECLS

#endif
