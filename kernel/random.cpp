/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    random.cpp
    Kernel entropy gathering.

*******************************************************************************/

#include <errno.h>

#include <sortix/clock.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/time.h>

namespace Sortix {

static unsigned long sequence = 0;

int sys_getentropy(void* user_buffer, size_t size)
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
		return errno = EIO, -1;
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
	if ( !CopyToUser(user_buffer, buffer, size) )
		return -1;
	return 0;
}

} // namespace Sortix
