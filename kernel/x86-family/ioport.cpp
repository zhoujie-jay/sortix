/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * x86-family/ioport.cpp
 * IO ports.
 */

#include <errno.h>
#include <timespec.h>

#include <sortix/clock.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/time.h>

namespace Sortix {

bool wait_inport8_clear(uint16_t ioport, uint8_t bits, bool any, unsigned int msecs)
{
	struct timespec timeout = timespec_make(msecs / 1000, (msecs % 1000) * 1000000L);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	struct timespec begun;
	clock->Get(&begun, NULL);
	while ( true )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		uint8_t reg_snapshop = inport8(ioport);
		if ( !any && (reg_snapshop & bits) == 0 )
			return true;
		if ( any && (reg_snapshop & bits) != bits )
			return true;
		struct timespec elapsed = timespec_sub(now, begun);
		if ( timespec_le(timeout, elapsed) )
			return errno = ETIMEDOUT, false;
		kthread_yield();
	}
}

bool wait_inport8_set(uint16_t ioport, uint8_t bits, bool any, unsigned int msecs)
{
	struct timespec timeout = timespec_make(msecs / 1000, (msecs % 1000) * 1000000L);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	struct timespec begun;
	clock->Get(&begun, NULL);
	while ( true )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		uint8_t reg_snapshop = inport8(ioport);
		if ( !any && (reg_snapshop & bits) == bits )
			return true;
		if ( any && (reg_snapshop & bits) != 0 )
			return true;
		struct timespec elapsed = timespec_sub(now, begun);
		if ( timespec_le(timeout, elapsed) )
			return errno = ETIMEDOUT, false;
		kthread_yield();
	}
}

} // namespace Sortix
