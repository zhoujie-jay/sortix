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
 * test-pthread-once.c
 * Tests whether basic pthread_once() support works.
 */

#include <pthread.h>

#include "test.h"

static int init_counter = 0;
static pthread_once_t init_counter_once = PTHREAD_ONCE_INIT;

void init_counter_increase(void)
{
	init_counter++;
}

void* thread_routine(void* ctx)
{
	(void) ctx;

	pthread_once(&init_counter_once, init_counter_increase);

	return NULL;
}

int main(void)
{
	int errnum;

	pthread_once(&init_counter_once, init_counter_increase);

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, NULL)) )
		test_error(errnum, "pthread_create");

	if ( (errnum = pthread_join(thread, NULL)) )
		test_error(errnum, "pthread_join");

	test_assert(init_counter == 1);

	return 0;
}
