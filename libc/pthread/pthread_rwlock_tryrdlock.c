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
 * pthread/pthread_rwlock_tryrdlock.c
 * Attempts to acquire read access to a read-write lock.
 */

#include <errno.h>
#include <pthread.h>

int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock)
{
	int ret;
	if ( (ret = pthread_mutex_trylock(&rwlock->request_mutex)) )
		return errno = ret;
	while ( rwlock->num_writers || rwlock->pending_writers )
	{
		pthread_mutex_unlock(&rwlock->request_mutex);
		return errno = EBUSY;
	}
	rwlock->num_readers++;
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
