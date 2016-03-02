/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * pthread/pthread_mutex_lock.c
 * Locks a mutex.
 */

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <timespec.h>

static const unsigned long UNLOCKED_VALUE = 0;
static const unsigned long LOCKED_VALUE = 1;

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	while ( !__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED_VALUE, LOCKED_VALUE) )
	{
		if ( mutex->type == PTHREAD_MUTEX_RECURSIVE &&
		     (pthread_t) mutex->owner == pthread_self() )
				return mutex->recursion++, 0;
		sched_yield();
	}
	mutex->owner = (unsigned long) pthread_self();
	mutex->recursion = 0;
	return 0;
}
