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
 * random.cpp
 * Kernel entropy gathering.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/clock.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/time.h>

namespace Sortix {
namespace Random {

static unsigned long sequence = 0;

bool HasEntropy()
{
	// We only have new entropy once and that's at boot.
	return sequence == 0;
}

void GetEntropy(void* result, size_t size)
{
	union
	{
		unsigned char buffer[256];
		struct
		{
			struct timespec realtime;
			struct timespec monotonic;
			unsigned long sequence;
		} seed;
	};
	if ( sizeof(buffer) < size )
		size = sizeof(buffer);
	// TODO: SECURITY: We need to actually gather entropy and deliver it.
	for ( size_t i = 0; i < size; i++ )
		buffer[i] = i;
	// NOTE: This is not random and is not meant to be random, this is just
	//       meant to make the returned entropy a little different each time
	//       until we have real randomness, even across reboots. The userland
	//       arc4random mixer will mix it around and the produced streams will
	//       look random and should not repeat in practice.
	seed.realtime = Time::Get(CLOCK_REALTIME);
	seed.monotonic = Time::Get(CLOCK_MONOTONIC);
	seed.sequence = InterlockedIncrement(&sequence).o;
	memcpy(result, buffer, size);
}

} // namespace Random
} // namespace Sortix

namespace Sortix {

int sys_getentropy(void* user_buffer, size_t size)
{
	unsigned char buffer[256];
	if ( sizeof(buffer) < size )
		return errno = EIO, -1;
	arc4random_buf(buffer, sizeof(buffer));
	if ( !CopyToUser(user_buffer, buffer, size) )
		return -1;
	return 0;
}

} // namespace Sortix
