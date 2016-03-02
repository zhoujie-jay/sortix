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
 * test-pthread-self.c
 * Tests whether basic pthread_self() support works.
 */

#include <pthread.h>

#include "test.h"

static pthread_t child_thread_self;

void* thread_routine(void* main_thread_ptr)
{
	pthread_t main_thread = *(pthread_t*) main_thread_ptr;

	test_assert(!pthread_equal(main_thread, pthread_self()));

	child_thread_self = pthread_self();

	return NULL;
}

int main(void)
{
	int errnum;

	pthread_t main_thread = pthread_self();

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, &main_thread)) )
		test_error(errnum, "pthread_create");

	if ( (errnum = pthread_join(thread, NULL)) )
		test_error(errnum, "pthread_join");

	test_assert(!pthread_equal(thread, pthread_self()));
	test_assert(pthread_equal(thread, child_thread_self));
	test_assert(!pthread_equal(pthread_self(), child_thread_self));

	return 0;
}
