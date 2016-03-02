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
 * time/clock.c
 * Get process execution time.
 */

#include <assert.h>
#include <time.h>

// TODO: This function is crap and has been replaced by clock_gettime.
clock_t clock(void)
{
	struct timespec now;
	if ( clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now) < 0 )
		return -1;
	static_assert(CLOCKS_PER_SEC == 1000000, "CLOCKS_PER_SEC == 1000000");
	return (clock_t) now.tv_sec * 1000000 + (clock_t) now.tv_sec / 1000;
}
