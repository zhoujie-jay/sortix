/*
 * Copyright (c) 2011, 2012, 2013 Jonas 'Sortie' Termansen.
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
 * time.cpp
 * Handles interrupts whenever some time has passed and provides useful
 * time-related functions to the kernel.
 */

#include <sys/types.h>

#include <errno.h>
#include <timespec.h>

#include <sortix/clock.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>

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
	case CLOCK_THREAD_CPUTIME_ID: return &CurrentThread()->execute_clock;
	case CLOCK_THREAD_SYSTIME_ID: return &CurrentThread()->system_clock;
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
	Thread* thread = CurrentThread();
	Process* process = thread->process;
	thread->execute_clock.Advance(tick_period);
	process->execute_clock.Advance(tick_period);
	process->child_execute_clock.Advance(tick_period);
	if ( system_mode )
	{
		thread->system_clock.Advance(tick_period);
		process->system_clock.Advance(tick_period);
		process->child_system_clock.Advance(tick_period);
	}
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
