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
 * user-timer.cpp
 * Timers that send signals when triggered.
 */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/signal.h>
#include <sortix/sigevent.h>
#include <sortix/time.h>
#include <sortix/tmns.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/user-timer.h>

namespace Sortix {

// TODO: We also need to fetch the pthread attr if there is one.
static bool FetchSigevent(struct sigevent* dst, const struct sigevent* src)
{
	if ( src )
		return CopyFromUser(dst, src, sizeof(struct sigevent));
	dst->sigev_notify = SIGEV_SIGNAL;
	dst->sigev_signo = SIGALRM;
	// TODO: "and the sigev_value member having the value of the timer ID."
	//  - Does POSIX want the caller to be psychic and should we write back the
	//    final default sigevent?
	return true;
}

static UserTimer* LookupUserTimer(Process* process, timer_t timerid)
{
	if ( PROCESS_TIMER_NUM_MAX <= timerid )
		return errno = EINVAL, (UserTimer*) NULL;
	UserTimer* user_timer = &process->user_timers[timerid];
	if ( !user_timer->timer.IsAttached() )
		return errno = EINVAL, (UserTimer*) NULL;
	return user_timer;
}

static Timer* LookupTimer(Process* process, timer_t timerid)
{
	UserTimer* user_timer = LookupUserTimer(process, timerid);
	return user_timer ? &user_timer->timer : (Timer*) NULL;
}

int sys_timer_create(clockid_t clockid,
                     struct sigevent* sigevp,
                     timer_t* timerid_ptr)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	Clock* clock = Time::GetClock(clockid);
	if ( !clock )
		return -1;

	struct sigevent sigev;
	if ( !FetchSigevent(&sigev, sigevp) )
		return -1;

	// Allocate a timer for this request.
	timer_t timerid;
	for ( timerid = 0; timerid < PROCESS_TIMER_NUM_MAX; timerid++ )
	{
		Timer* timer = &process->user_timers[timerid].timer;
		if ( timer->IsAttached() )
			continue;
		timer->Attach(clock);
		break;
	}

	if ( PROCESS_TIMER_NUM_MAX <= timerid )
		return -1;

	if ( !CopyToUser(timerid_ptr, &timerid, sizeof(timerid)) )
	{
		process->user_timers[timerid].timer.Detach();
		return -1;
	}

	process->user_timers[timerid].process = process;
	process->user_timers[timerid].event = sigev;
	process->user_timers[timerid].timerid = timerid;

	return 0;
}

int sys_timer_delete(timer_t timerid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	Timer* timer = LookupTimer(process, timerid);
	if ( !timer )
		return -1;

	timer->Cancel();
	timer->Detach();

	return 0;
}

int sys_timer_getoverrun(timer_t timerid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	Timer* timer = LookupTimer(process, timerid);
	if ( !timer )
		return -1;

	if ( (size_t) INT_MAX < timer->num_overrun_events)
		return INT_MAX;

	// TODO: How does the caller reset the overrun count back to 0? Should we
	//       adopt the Linux semantics where it resets back to 0 after INT_MAX?
	//       How about signed overflow in the kernel and in the user process?

	return 0;
}

int sys_timer_gettime(timer_t timerid, struct itimerspec* user_value)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	Timer* timer = LookupTimer(process, timerid);
	if ( !timer )
		return -1;

	struct itimerspec value;
	timer->Get(&value);

	return CopyToUser(user_value, &value, sizeof(value)) ? 0 : -1;
}

static void timer_callback(Clock* /*clock*/, Timer* timer, void* user)
{
	UserTimer* user_timer = (UserTimer*) user;
	Process* process = user_timer->process;
	ScopedLock lock(&process->user_timers_lock);

	// TODO: This delivery facility is insufficient! sigevent is much more
	//       powerful than sending a simple old-school signal.
	if ( !process->DeliverSignal(user_timer->event.sigev_signo) &&
	      errno == ESIGPENDING )
	{
		if ( timer->num_overrun_events < SIZE_MAX )
			timer->num_overrun_events++;
	}
}

int sys_timer_settime(timer_t timerid,
                      int flags,
                      const struct itimerspec* user_value,
                      struct itimerspec* user_ovalue)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	UserTimer* user_timer = LookupUserTimer(process, timerid);
	if ( !user_timer )
		return -1;

	Timer* timer = &user_timer->timer;

	struct itimerspec value, ovalue;
	if ( !CopyFromUser(&value, user_value, sizeof(value)) )
		return -1;

	if ( timespec_lt(value.it_value, timespec_nul()) ||
	     timespec_lt(value.it_interval, timespec_nul()) ||
	     (flags & ~(TIMER_ABSTIME)) != 0 )
		return errno = EINVAL, -1;

	int timer_flags = 0;
	if ( flags & TIMER_ABSTIME )
		timer_flags |= TIMER_ABSOLUTE;

	timer->Set(&value, &ovalue, timer_flags, timer_callback, user_timer);

	// Let the caller know how much time was left on the timer.
	if ( user_ovalue && !CopyToUser(user_ovalue, &ovalue, sizeof(ovalue)) )
		return -1;

	return 0;
}

int sys_clock_gettimeres(clockid_t clockid,
                         struct timespec* time,
                         struct timespec* res)
{
	Clock* clock = Time::GetClock(clockid);
	if ( !clock )
		return -1;

	struct timespec ktime, kres;
	clock->Get(&ktime, &kres);

	return (!time || CopyToUser(time, &ktime, sizeof(ktime))) &&
	       (!res || CopyToUser(res, &kres, sizeof(kres))) ? 0 : -1;
}

int sys_clock_settimeres(clockid_t clockid,
                         const struct timespec* time,
                         const struct timespec* res)
{
	Clock* clock = Time::GetClock(clockid);
	if ( !clock )
		return -1;

	struct timespec ktime, kres;
	if ( (time && !CopyFromUser(&ktime, time, sizeof(ktime))) ||
	     (res && !CopyFromUser(&kres, res, sizeof(kres))) )
		return -1;

	clock->Set(time ? &ktime : NULL, res ? &kres : NULL);

	return 0;
}

int sys_clock_nanosleep(clockid_t clockid,
                        int flags,
                        const struct timespec* user_duration,
                        struct timespec* user_remainder)
{
	struct timespec time;

	Clock* clock = Time::GetClock(clockid);
	if ( !clock )
		return -1;

	if ( !CopyFromUser(&time, user_duration, sizeof(time)) )
		return -1;

	time = flags & TIMER_ABSTIME ? clock->SleepUntil(time) :
	                               clock->SleepDelay(time);

	if ( user_remainder && !CopyToUser(user_remainder, &time, sizeof(time)) )
		return -1;

	return timespec_eq(time, timespec_nul()) ? 0 : (errno = EINTR, -1);
}

int sys_timens(struct tmns* user_tmns)
{
	Clock* execute_clock = Time::GetClock(CLOCK_PROCESS_CPUTIME_ID);
	Clock* system_clock = Time::GetClock(CLOCK_PROCESS_SYSTIME_ID);
	Clock* child_execute_clock = Time::GetClock(CLOCK_CHILD_CPUTIME_ID);
	Clock* child_system_clock = Time::GetClock(CLOCK_CHILD_SYSTIME_ID);

	// Note: It is safe to access the clocks in this manner as each of them are
	//       locked by disabling interrupts. This is perhaps not SMP-ready, but
	//       it will do for now.
	struct tmns tmns;
	Interrupt::Disable();
	tmns.tmns_utime = execute_clock->current_time;
	tmns.tmns_stime = system_clock->current_time;
	tmns.tmns_cutime = child_execute_clock->current_time;
	tmns.tmns_cstime = child_system_clock->current_time;
	Interrupt::Enable();

	return CopyToUser(user_tmns, &tmns, sizeof(tmns)) ? 0 : -1;
}

} // namespace Sortix
