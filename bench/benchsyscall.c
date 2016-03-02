/*
 * Copyright (c) 2011 Jonas 'Sortie' Termansen.
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
 * benchsyscall.c
 * Benchmarks the speed of system calls.
 */

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

static int uptime(uintmax_t* usecs)
{
	struct timespec uptime;
	if ( clock_gettime(CLOCK_BOOT, &uptime) < 0 )
		return -1;
	*usecs = uptime.tv_sec * 1000000ULL + uptime.tv_nsec / 1000ULL;
	return 0;
}

int main(void)
{
	uintmax_t start;
	if ( uptime(&start) )
		err(1, "uptime");
	uintmax_t end = start + 1ULL * 1000ULL * 1000ULL; // 1 second
	size_t count = 0;
	uintmax_t now;
	while ( !uptime(&now) && now < end ) { count++; }
	printf("Made %zu system calls in 1 second\n", count);
	return 0;
}
