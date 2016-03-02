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
 * timespec/timespec.c
 * Utility functions for manipulation of struct timespec.
 */

#include <timespec.h>

static const long NANOSECONDS_PER_SECOND = 1000000000L;

// TODO: The C modulo operator doesn't do exactly what we desire.
static long proper_modulo(long a, long b)
{
	if ( 0 <= a )
		return a % b;
	long tmp = - (unsigned long) a % b;
	return tmp ? tmp : 0;
}

struct timespec timespec_canonalize(struct timespec t)
{
	t.tv_sec += t.tv_nsec / NANOSECONDS_PER_SECOND;
	t.tv_nsec = proper_modulo(t.tv_nsec, NANOSECONDS_PER_SECOND);
	return t;
}

struct timespec timespec_add(struct timespec a, struct timespec b)
{
	struct timespec ret;
	a = timespec_canonalize(a);
	b = timespec_canonalize(b);
	ret.tv_sec = a.tv_sec + b.tv_sec;
	ret.tv_nsec = a.tv_nsec + b.tv_nsec;
	return timespec_canonalize(ret);
}

struct timespec timespec_sub(struct timespec a, struct timespec b)
{
	a = timespec_canonalize(a);
	b = timespec_canonalize(b);
	return timespec_add(a, timespec_neg(b));
}
