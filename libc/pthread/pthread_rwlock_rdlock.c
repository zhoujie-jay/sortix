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
 * pthread/pthread_rwlock_rdlock.c
 * Acquires read access to a read-write lock.
 */

#include <pthread.h>

int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
	pthread_mutex_lock(&rwlock->request_mutex);
	rwlock->pending_readers++;
	while ( rwlock->num_writers || rwlock->pending_writers )
		pthread_cond_wait(&rwlock->reader_condition, &rwlock->request_mutex);
	rwlock->pending_readers--;
	rwlock->num_readers++;
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
