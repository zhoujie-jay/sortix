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
 * test-pthread-basic.c
 * Tests whether basic pthread support works.
 */

#include <pthread.h>

#include "test.h"

void* thread_routine(void* cookie)
{
	int* test_failure_ptr = (int*) cookie;
	*test_failure_ptr = 0;
	return cookie;
}

int main(void)
{
	int errnum;

	int test_failure = 1;

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, &test_failure)) )
		test_error(errnum, "pthread_create");

	void* thread_result;
	if ( (errnum = pthread_join(thread, &thread_result)) )
		test_error(errnum, "pthread_join");

	test_assert(test_failure == 0);
	test_assert(thread_result == &test_failure);

	return 0;
}
