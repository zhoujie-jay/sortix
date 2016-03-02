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
 * signal/sigismember.c
 * Test signal membership in a signal set.
 */

#include <errno.h>
#include <signal.h>
#include <stdint.h>

int sigismember(const sigset_t* set, int signum)
{
	int max_signals = sizeof(set->__val) * 8;
	if ( signum < 0 || max_signals <= signum )
		return errno = EINVAL, -1;
	size_t which_byte = signum / 8;
	size_t which_bit  = signum % 8;
	const uint8_t* bytes = (const uint8_t*) set->__val;
	return bytes[which_byte] & (1 << which_bit) ? 1 : 0;
}
