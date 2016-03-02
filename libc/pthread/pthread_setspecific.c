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
 * pthread/pthread_setspecific.c
 * Thread-specific data management.
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

int pthread_setspecific(pthread_key_t key, const void* value_const)
{
	void* value = (void*) value_const;

	struct pthread* thread = pthread_self();

	if ( key < thread->keys_length )
		return thread->keys[key] = value, 0;

	pthread_mutex_lock(&__pthread_keys_lock);

	assert(key < __pthread_keys_length);
	assert(__pthread_keys[key].destructor);

	pthread_mutex_unlock(&__pthread_keys_lock);

	size_t old_length = thread->keys_length;
	size_t new_length = __pthread_keys_length;
	size_t new_size = new_length * sizeof(void*);
	void** new_keys = (void**) realloc(thread->keys, new_size);
	if ( !new_keys )
		return errno;
	thread->keys = new_keys;
	thread->keys_length = new_length;
	for ( size_t i = old_length; i < new_length; i++ )
		new_keys[i] = NULL;

	return thread->keys[key] = value, 0;
}
