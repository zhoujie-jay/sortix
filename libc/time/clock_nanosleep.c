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
 * time/clock_nanosleep.c
 * Sleep for a duration on a clock.
 */

#include <sys/syscall.h>

#include <time.h>

DEFN_SYSCALL4(int, sys_clock_nanosleep, SYSCALL_CLOCK_NANOSLEEP, clockid_t,
              int, const struct timespec*, struct timespec*);

int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec* duration, struct timespec* remainder)
{
	return sys_clock_nanosleep(clockid, flags, duration, remainder);
}
