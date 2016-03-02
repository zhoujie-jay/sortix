/*
 * Copyright (c) 201 Jonas 'Sortie' Termansen.
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
 * sys/times/times.c
 * Get process execution time statistics.
 */

#include <sys/times.h>

#include <stdint.h>
#include <time.h>
#include <unistd.h>

// TODO: It is a bit needless to use 64-bit integers on 32-bit platforms and the
//       division is probably very inefficient. In addition, this implementation
//       doesn't hanndle a full 64-bit clock_t, however it won't cause problems
//       before a lot of years has passed, which is unlikely to happen, and the
//       clock function probably shouldn't be used in programs with that long
//       running times.
// TODO: Does this function overflow correctly?
clock_t timespec_to_clock(struct timespec t)
{
	uint64_t ticks_per_second = (uint64_t) sysconf(_SC_CLK_TCK);
	uint64_t sec_contrib = (uint64_t) t.tv_sec * ticks_per_second;
	uint64_t nsec_contrib = (uint64_t) t.tv_nsec * ticks_per_second;
	return (clock_t) (sec_contrib + nsec_contrib / 1000000000);
}

// TODO: This function is crap and has been replaced by timens or clock_gettime.
clock_t times(struct tms* buf)
{
	if ( buf )
	{
		struct tmns tmns;
		if ( timens(&tmns) < 0 )
			return (clock_t) -1;
		buf->tms_utime = timespec_to_clock(tmns.tmns_utime);
		buf->tms_stime = timespec_to_clock(tmns.tmns_stime);
		buf->tms_cutime = timespec_to_clock(tmns.tmns_cutime);
		buf->tms_cstime = timespec_to_clock(tmns.tmns_cstime);
	}

	// TODO: Should this be CLOCK_REALTIME instead?
	struct timespec now;
	if ( clock_gettime(CLOCK_MONOTONIC, &now) < 0 )
		return -1;
	return timespec_to_clock(now);
}
