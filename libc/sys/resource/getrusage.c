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
 * sys/resource/getrusage.c
 * Get resource usage statistics.
 */

#include <sys/resource.h>
#include <sys/syscall.h>

#include <errno.h>
#include <time.h>
#include <timespec.h>

static struct timeval timeval_of_timespec(struct timespec ts)
{
	struct timeval tv;
	tv.tv_sec = ts.tv_sec;
	tv.tv_usec = ts.tv_nsec / 1000;
	return tv;
}

int getrusage(int who, struct rusage* usage)
{
	struct tmns tmns;
	if ( timens(&tmns) != 0 )
		return -1;
	if ( who == RUSAGE_SELF )
	{
		usage->ru_utime = timeval_of_timespec(tmns.tmns_utime);
		usage->ru_stime = timeval_of_timespec(tmns.tmns_stime);
	}
	else if ( who == RUSAGE_CHILDREN )
	{
		usage->ru_utime = timeval_of_timespec(tmns.tmns_cutime);
		usage->ru_stime = timeval_of_timespec(tmns.tmns_cstime);
	}
	else
		return errno = EINVAL, -1;
	return 0;
}
