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
 * pthread/pthread_once.c
 * Dynamic package initialization.
 */

#include <pthread.h>

int pthread_once(pthread_once_t* once_control, void (*init_routine)(void))
{
	pthread_mutex_lock(&once_control->lock);
	if ( !once_control->executed )
	{
		// TODO: If this thread is a cancellation point and is canceled, we must
		//       pretend this pthread_once was never called. This means that we
		//       need to ensure that once_control->lock is unlocked in the event
		//       of a cancellation.
		init_routine();
		once_control->executed = 1;
	}
	pthread_mutex_unlock(&once_control->lock);
	return 0;
}
