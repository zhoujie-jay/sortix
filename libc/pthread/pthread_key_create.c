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
 * pthread/pthread_key_create.c
 * Thread-specific data key creation.
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

static void noop_destructor(void* ctx)
{
	(void) ctx;
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
	if ( !destructor )
		destructor = noop_destructor;

	pthread_mutex_lock(&__pthread_keys_lock);

	if ( __pthread_keys_used < __pthread_keys_length )
	{
		size_t result_key = 0;
		while ( __pthread_keys[result_key].destructor )
			result_key++;
		__pthread_keys[__pthread_keys_used++, *key = result_key].destructor = destructor;
	}
	else
	{
		size_t old_length = __pthread_keys_length;
		size_t new_length = old_length ? old_length * 2 : 8;
		size_t new_size = sizeof(struct pthread_key) * new_length;
		struct pthread_key* new_keys =
			(struct pthread_key*) realloc(__pthread_keys, new_size);
		if ( !new_keys )
		{
			pthread_mutex_unlock(&__pthread_keys_lock);
			return errno;
		}
		__pthread_keys = new_keys;
		__pthread_keys_length = new_length;
		for ( size_t i = old_length; i < new_length; i++ )
			new_keys[i].destructor = NULL;

		__pthread_keys[__pthread_keys_used++, *key = old_length].destructor = destructor;
	}

	pthread_mutex_unlock(&__pthread_keys_lock);

	return 0;
}
