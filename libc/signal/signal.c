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
 * signal/signal.c
 * Configure and retrieve a signal handler.
 */

#include <signal.h>
#include <string.h>

void (*signal(int signum, void (*handler)(int)))(int)
{
	// Create a structure describing the new handler.
	struct sigaction newact;
	memset(&newact, 0, sizeof(newact));
	sigemptyset(&newact.sa_mask);
	newact.sa_handler = handler;
	newact.sa_flags = SA_RESTART;

	// Register the new handler and atomically get the old.
	struct sigaction oldact;
	if ( sigaction(signum, &newact, &oldact) != 0 )
		return SIG_ERR;

	// We can't return the old handler properly if it's SA_SIGINFO or SA_COOKIE,
	// unless it's the common SIG_IGN or SIG_DFL handlers. Let's just say to the
	// caller that it's SIG_DFL and assume that they'll be using sigaction
	// instead if they wish to restore an old handler.
	if ( (oldact.sa_flags & (SA_SIGINFO | SA_COOKIE)) &&
	     oldact.sa_handler != SIG_IGN &&
	     oldact.sa_handler != SIG_DFL )
		return SIG_DFL;

	return oldact.sa_handler;
}
