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
 * test-signal-raise.c
 * Tests whether basic raise() support works.
 */

#include <signal.h>

#include "test.h"

static int signal_counter = 0;

void signal_handler(int signum, siginfo_t* siginfo, void* ucontext_ptr)
{
	(void) signum;
	(void) siginfo;
	(void) ucontext_ptr;

	test_assert(signum == SIGUSR1);

	signal_counter++;
}

int main(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = signal_handler;
	sa.sa_flags = SA_SIGINFO;

	if ( sigaction(SIGUSR1, &sa, NULL) )
		test_error(errno, "sigaction(USR1)");

	raise(SIGUSR1);

	test_assert(signal_counter == 1);

	return 0;
}
