/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * pthread/pthread_cond_timedwait.c
 * Waits on a condition or until a timeout happens.
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

int pthread_cond_timedwait(pthread_cond_t* restrict cond,
                           pthread_mutex_t* restrict mutex,
                           const struct timespec* restrict abstime)
{
	struct pthread_cond_elem elem;
	elem.next = NULL;
	elem.woken = 0;
	if ( cond->last )
		cond->last->next = &elem;
	if ( !cond->last )
		cond->first = &elem;
	cond->last = &elem;
	while ( !elem.woken )
	{
		struct timespec now;
		if ( clock_gettime(cond->clock, &now) < 0 )
			return errno;
		if ( timespec_le(*abstime, now) )
			return errno = ETIMEDOUT;
		pthread_mutex_unlock(mutex);
		sched_yield();
		pthread_mutex_lock(mutex);
	}
	return 0;
}
