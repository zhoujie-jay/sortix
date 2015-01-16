/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    x86-family/ioport.cpp
    IO ports.

*******************************************************************************/

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
