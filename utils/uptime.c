/*
 * Copyright (c) 2011, 2012, 2013 Jonas 'Sortie' Termansen.
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
 * uptime.c
 * Tell how long the system has been running.
 */

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

size_t Seconds(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL)) % (60ULL);
}

size_t Minutes(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL)) % (60ULL);
}

size_t Hours(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL * 60ULL)) % (24ULL);
}

size_t Days(uintmax_t usecs)
{
	return usecs / (1000ULL * 1000ULL * 60ULL * 60ULL * 24ULL);
}

void PrintElement(size_t num, const char* single, const char* multiple)
{
	static const char* prefix = "";
	if ( !num ) { return; }
	const char* str = (num>1) ? multiple : single;
	printf("%s%zu %s", prefix, num, str);
	prefix = ", ";
}

int main(void)
{
	struct timespec uptime;
	if ( clock_gettime(CLOCK_BOOT, &uptime) < 0 )
		error(1, errno, "clock_gettime(CLOCK_BOOT)");

	uintmax_t usecssinceboot = uptime.tv_sec * 1000000ULL +
	                           uptime.tv_nsec / 1000ULL;
	PrintElement(Days(usecssinceboot), "day", "days");
	PrintElement(Hours(usecssinceboot), "hour", "hours");
	PrintElement(Minutes(usecssinceboot), "min", "mins");
	PrintElement(Seconds(usecssinceboot), "sec", "secs");
	printf("\n");

	return 0;
}
