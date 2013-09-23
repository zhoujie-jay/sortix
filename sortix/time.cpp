/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    time.cpp
    Handles interrupts whenever some time has passed and provides useful
    time-related functions to the kernel.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <timespec.h>

#include <sortix/clock.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/clock.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/scheduler.h>

#ifdef PLATFORM_SERIAL
#include "serialterminal.h"
#endif

namespace Sortix {
namespace Time {

void CPUInit();

Clock* realtime_clock;
Clock* uptime_clock;

Clock* GetClock(clockid_t clock)
{
	switch ( clock )
	{
	case CLOCK_REALTIME: return realtime_clock;
	case CLOCK_BOOT: return uptime_clock;
	case CLOCK_INIT: return uptime_clock;
	case CLOCK_MONOTONIC: return uptime_clock;
	case CLOCK_PROCESS_CPUTIME_ID: return &CurrentProcess()->execute_clock;
	case CLOCK_PROCESS_SYSTIME_ID: return &CurrentProcess()->system_clock;
	case CLOCK_CHILD_CPUTIME_ID: return &CurrentProcess()->child_execute_clock;
	case CLOCK_CHILD_SYSTIME_ID: return &CurrentProcess()->child_system_clock;
	default: return errno = ENOTSUP, (Clock*) NULL;
	}
}

struct timespec Get(clockid_t clockid)
{
	Clock* clock = GetClock(clockid);
	if ( clock )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		return now;
	}
	return timespec_nul();
}

void OnTick(struct timespec tick_period, bool system_mode)
{
	realtime_clock->Advance(tick_period);
	uptime_clock->Advance(tick_period);
	Process* process = CurrentProcess();
	process->execute_clock.Advance(tick_period);
	process->child_execute_clock.Advance(tick_period);
	if ( system_mode )
		process->system_clock.Advance(tick_period),
		process->child_system_clock.Advance(tick_period);
}

void Init()
{
	if ( !(realtime_clock = new Clock()) )
		Panic("Unable to allocate realtime clock");
	if ( !(uptime_clock = new Clock()) )
		Panic("Unable to allocate uptime clock");
	CPUInit();
}

} // namespace Time
} // namespace Sortix
