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
 * test-pthread-main-join.c
 * Tests whether the main thread can be joined.
 */

#include <pthread.h>

#include "test.h"

pthread_t main_thread;

void* thread_routine(void* expected_result)
{
	int errnum;

	void* main_thread_result;
	if ( (errnum = pthread_join(main_thread, &main_thread_result)) )
		test_error(errnum, "pthread_join");

	test_assert(expected_result == &main_thread);

	exit(0);
}

int main(void)
{
	int errnum;

	main_thread = pthread_self();

	void* expected_result = &main_thread;

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, expected_result)) )
		test_error(errnum, "pthread_create");

	pthread_exit(expected_result);
}
