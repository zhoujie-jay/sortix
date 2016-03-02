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
 * pthread/pthread_exit.c
 * Exits the current thread.
 */

#include <sys/mman.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

__attribute__((__noreturn__))
void pthread_exit(void* return_value)
{
	struct pthread* thread = pthread_self();

	pthread_mutex_lock(&__pthread_keys_lock);
	bool keys_left = true;
	while ( keys_left )
	{
		keys_left = false;
		for ( pthread_key_t key = 0; key < thread->keys_length; key++ )
		{
			void* key_value = thread->keys[key];
			if ( !key_value )
				continue;
			thread->keys[key] = NULL;
			if ( __pthread_keys[key].destructor )
				__pthread_keys[key].destructor(key_value);
			keys_left = true;
		}
	}
	free(thread->keys);
	thread->keys = NULL;
	thread->keys_length = 0;
	pthread_mutex_unlock(&__pthread_keys_lock);

	pthread_mutex_lock(&thread->detach_lock);
	thread->exit_result = return_value;
	int exit_flags = EXIT_THREAD_UNMAP;
	struct exit_thread extended;
	memset(&extended, 0, sizeof(extended));
	extended.unmap_from = thread->uthread.stack_mmap;
	extended.unmap_size = thread->uthread.stack_size;
	if ( thread->detach_state == PTHREAD_CREATE_JOINABLE )
	{
		extended.zero_from = &thread->join_lock.lock;
		extended.zero_size = sizeof(thread->join_lock.lock);
		exit_flags |= EXIT_THREAD_ZERO;
	}
	else
	{
		extended.tls_unmap_from = thread->uthread.tls_mmap;
		extended.tls_unmap_size = thread->uthread.tls_size;
		exit_flags |= EXIT_THREAD_TLS_UNMAP;
	}

	exit_thread(0, EXIT_THREAD_ONLY_IF_OTHERS | exit_flags, &extended);
	exit(0);
}
