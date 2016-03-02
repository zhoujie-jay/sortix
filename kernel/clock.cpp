/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * clock.cpp
 * Clock and timer facility.
 */

#include <assert.h>
#include <timespec.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/timer.h>
#include <sortix/kernel/worker.h>

namespace Sortix {

Clock::Clock()
{
	delay_timer = NULL;
	absolute_timer = NULL;
	current_time = timespec_nul();
	current_advancement = timespec_nul();
	resolution = timespec_nul();
	clock_mutex = KTHREAD_MUTEX_INITIALIZER;
	clock_callable_from_interrupt = false;
	we_disabled_interrupts = false;
}

Clock::~Clock()
{
	// TODO: The best solution would probably be to cancel everything that is
	//       waiting on us, but that is a bit dangerous since things have to be
	//       notified carefully that they should not use stale pointers to this
	//       clock. This is a bunch of work and since the clock is being
	//       destroyed, you could argue that you shouldn't be using a clock
	//       whose lifetime you don't control. Therefore assume that all users
	//       of the clock has stopped using it.
	assert(!absolute_timer && !delay_timer);
}

// This clock and timer facility is designed to work even from interrupt
// handlers. For instance, this is needed by the uptime clock that is
// incremented every timer interrupt. If we don't need interrupt handler safety,
// we simply fall back on regular mutual exclusion.

void Clock::SetCallableFromInterrupts(bool callable_from_interrupts)
{
	clock_callable_from_interrupt = callable_from_interrupts;
}

void Clock::LockClock()
{
	if ( clock_callable_from_interrupt )
	{
		if ( (we_disabled_interrupts = Interrupt::IsEnabled()) )
			Interrupt::Disable();
	}
	else
		kthread_mutex_lock(&clock_mutex);
}

void Clock::UnlockClock()
{
	if ( clock_callable_from_interrupt )
	{
		if ( we_disabled_interrupts )
			Interrupt::Enable();
	}
	else
		kthread_mutex_unlock(&clock_mutex);
}

void Clock::Set(struct timespec* now, struct timespec* res)
{
	LockClock();

	if ( now )
		current_time = *now;
	if ( res )
		resolution = *res;

	TriggerAbsolute();

	UnlockClock();
}

void Clock::Get(struct timespec* now, struct timespec* res)
{
	LockClock();

	if ( now )
		*now = current_time;
	if ( res )
		*res = resolution;

	UnlockClock();
}

// We maintain two queues of timers; one for timers that sleep for a duration
// and one that that sleeps until a certain point in time. This lets us deal
// nicely with non-monotonic clocks and simplifies the code. The absolute timers
// queue is simply sorted after their wake-up time, while the delay timers queue
// is sorted after their delays, where each node stores the delay between it and
// its previous node (if any, otherwise just the actual time left of the timer).
// This data structure allows constant time detection of whether a timer should
// be fired and the double-linked queue allow constant-time cancellation - this
// is at the expense of linear time insertion, but it is kinda okay since timers
// that are soon will always be at the start (and hence quick to insert), while
// timers in the far future will be last and the calling thread probably
// wouldn't mind a little delay.

// TODO: If locking the clock means disabling interrupts, and a large numbers of
//       timers are attached to this clock, then inserting a timer becomes
//       expensive as the CPU locks up for a moment. Perhaps this is not as bad
//       as it theoretically could be?

void Clock::RegisterAbsolute(Timer* timer) // Lock acquired.
{
	assert(!(timer->flags & TIMER_ACTIVE));
	timer->flags |= TIMER_ACTIVE;

	Timer* before = NULL;
	for ( Timer* iter = absolute_timer; iter; iter = before->next_timer )
		if ( timespec_lt(timer->value.it_value, iter->value.it_value) )
			break;
		else
			before = iter;

	timer->prev_timer = before;
	timer->next_timer = before ? before->next_timer : NULL;
	if ( timer->next_timer ) timer->next_timer->prev_timer = timer;
	(before ? before->next_timer : absolute_timer) = timer;
}

void Clock::RegisterDelay(Timer* timer) // Lock acquired.
{
	assert(!(timer->flags & TIMER_ACTIVE));
	timer->flags |= TIMER_ACTIVE;

	Timer* before = NULL;
	struct timespec before_time = timespec_nul();
	for ( Timer* iter = delay_timer; iter; iter = before->next_timer )
		if ( timespec_lt(timer->value.it_value, iter->value.it_value) )
			break;
		else
			before = iter,
			before_time = timespec_add(before_time, iter->value.it_value);

	timer->value.it_value = timespec_sub(timer->value.it_value, before_time);
	timer->prev_timer = before;
	timer->next_timer = before ? before->next_timer : NULL;
	if ( timer->next_timer ) timer->next_timer->prev_timer = timer;
	(before ? before->next_timer : delay_timer) = timer;

	if ( timer->next_timer )
		timer->next_timer->value.it_value =
			timespec_sub(timer->next_timer->value.it_value, timer->value.it_value);
}

void Clock::Register(Timer* timer)
{
	if ( timer->flags & TIMER_ABSOLUTE )
		RegisterAbsolute(timer);
	else
		RegisterDelay(timer);
}

void Clock::UnlinkAbsolute(Timer* timer) // Lock acquired.
{
	(timer->prev_timer ? timer->prev_timer->next_timer : absolute_timer) = timer->next_timer;
	if ( timer->next_timer ) timer->next_timer->prev_timer = timer->prev_timer;
	timer->prev_timer = timer->next_timer = NULL;
	timer->flags &= ~TIMER_ACTIVE;
}

void Clock::UnlinkDelay(Timer* timer) // Lock acquired.
{
	(timer->prev_timer ? timer->prev_timer->next_timer : delay_timer) = timer->next_timer;
	if ( timer->next_timer ) timer->next_timer->prev_timer = timer->prev_timer;
	if ( timer->next_timer ) timer->next_timer->value.it_value = timespec_add(timer->next_timer->value.it_value, timer->value.it_value);
	timer->prev_timer = timer->next_timer = NULL;
	timer->flags &= ~TIMER_ACTIVE;
}

void Clock::Unlink(Timer* timer) // Lock acquired.
{
	if ( timer->flags & TIMER_ACTIVE )
	{
		if ( timer->flags & TIMER_ABSOLUTE )
			UnlinkAbsolute(timer);
		else
			UnlinkDelay(timer);
	}
}

void Clock::Cancel(Timer* timer)
{
	LockClock();

	Unlink(timer);

	while ( timer->flags & TIMER_FIRING )
	{
		UnlockClock();

		// TODO: This busy-loop is rather inefficient. We could set up some
		//       condition variable and wait on it. However, if the lock is
		//       turning interrupts off, then there is no mutex we can use.
		kthread_yield();

		LockClock();
	}

	UnlockClock();
}

// TODO: We need some method for threads to sleep for real but still be
//       interrupted by signals.
struct timespec Clock::SleepDelay(struct timespec duration)
{
	struct timespec start_advancement;
	struct timespec elapsed = timespec_nul();
	bool start_advancement_set = false;

	while ( timespec_lt(elapsed, duration) )
	{
		if ( start_advancement_set )
		{
			if ( Signal::IsPending() )
				return duration;

			kthread_yield();
		}

		LockClock();

		if ( !start_advancement_set )
			start_advancement = current_advancement,
			start_advancement_set = true;

		elapsed = timespec_sub(current_advancement, start_advancement);

		UnlockClock();
	}

	return timespec_nul();
}

// TODO: We need some method for threads to sleep for real but still be
//       interrupted by signals.
struct timespec Clock::SleepUntil(struct timespec expiration)
{
	while ( true )
	{
		LockClock();
		struct timespec now = current_time;
		UnlockClock();

		if ( timespec_le(expiration, now) )
			break;

		if ( Signal::IsPending() )
			return timespec_sub(expiration, now);

		kthread_yield();
	}

	return timespec_nul();
}

void Clock::Advance(struct timespec duration)
{
	LockClock();

	current_time = timespec_add(current_time, duration);
	current_advancement = timespec_add(current_advancement, duration);
	TriggerDelay(duration);
	TriggerAbsolute();

	UnlockClock();
}

// Fire timers that wait for a certain amount of time.
void Clock::TriggerDelay(struct timespec unaccounted) // Lock acquired.
{
	while ( Timer* timer = delay_timer )
	{
		if ( timespec_lt(unaccounted, timer->value.it_value) )
		{
			timer->value.it_value = timespec_sub(timer->value.it_value, unaccounted);
			break;
		}
		unaccounted = timespec_sub(unaccounted, timer->value.it_value);
		timer->value.it_value = timespec_nul();
		if ( (delay_timer = delay_timer->next_timer) )
			delay_timer->prev_timer = NULL;
		FireTimer(timer);
	}
}

// Fire timers that wait until a certain point in time.
void Clock::TriggerAbsolute() // Lock acquired.
{
	while ( Timer* timer = absolute_timer )
	{
		if ( timespec_lt(current_time, timer->value.it_value) )
			break;
		if ( (absolute_timer = absolute_timer->next_timer) )
			absolute_timer->prev_timer = NULL;
		FireTimer(timer);
	}
}

static void Clock__DoFireTimer(Timer* timer)
{
	timer->callback(timer->clock, timer, timer->user);
}

static void Clock__FireTimer(void* timer_ptr)
{
	Timer* timer = (Timer*) timer_ptr;
	assert(timer->clock);

	// Combine all the additionally pending events into a single one and notify
	// the caller of all the events that he missed because we couldn't call him
	// fast enough.
	timer->clock->LockClock();
	timer->num_overrun_events = timer->num_firings_scheduled;
	timer->num_firings_scheduled = 0;
	timer->clock->UnlockClock();

	Clock__DoFireTimer(timer);

	// If additional events happened during the time of the event handler, we'll
	// have to handle them because the firing bit is set. We'll schedule another
	// worker thread job and resume there, so this worker thread can continue to
	// do other important stuff.
	timer->clock->LockClock();
	if ( timer->num_firings_scheduled )
		Worker::Schedule(Clock__FireTimer, timer_ptr);
	// If this was the last event, we'll clear the firing bit and the advance
	// thread now has the responsibility of creating worker thread jobs.
	else
		timer->flags &= ~TIMER_FIRING;
	timer->clock->UnlockClock();
}

static void Clock__FireTimer_InterruptWorker(void* timer_ptr, void*, size_t)
{
	Clock__FireTimer(timer_ptr);
}

void Clock::FireTimer(Timer* timer)
{
	timer->flags &= ~TIMER_ACTIVE;

	// If the CPU is currently interrupted, we call the timer callback directly
	// only if it is known to work when the interrupts are disabled on this CPU.
	// Otherwise, we forward the timer pointer to a special interrupt-safe
	// worker thread that'll run the callback normally.
	if ( !Interrupt::IsEnabled() )
	{
		if ( timer->flags & TIMER_FUNC_INTERRUPT_HANDLER )
			Clock__DoFireTimer(timer);
		else if ( timer->flags & TIMER_FIRING )
			timer->num_firings_scheduled++;
		else
		{
			timer->flags |= TIMER_FIRING;
			Interrupt::ScheduleWork(Clock__FireTimer_InterruptWorker, timer, NULL, 0);
		}
	}

	// Normally, we will run the timer callback in a worker thread, but as an
	// optimization, if the callback is known to be short and simple and safely
	// handles this situation, we'll simply call it from the current thread.
	else
	{
		if ( timer->flags & TIMER_FUNC_ADVANCE_THREAD )
			Clock__DoFireTimer(timer);
		else if ( timer->flags & TIMER_FIRING )
			timer->num_firings_scheduled++;
		else
		{
			timer->flags |= TIMER_FIRING;
			Worker::Schedule(Clock__FireTimer, timer);
		}
	}

	// Rearm the timer only if it is periodic.
	if ( timespec_le(timer->value.it_interval, timespec_nul()) )
		return;

	// TODO: If the period is too short (such a single nanosecond) on a delay
	//       timer, then it will try to spend each nanosecond avanced carefully
	//       and reliably schedule a shitload of firings. Not only that, but it
	//       will also loop this function many million timers per tick!

	// TODO: Throtte the timer if firing while the callback is still running!
	// TODO: Doesn't reload properly for absolute timers!
	if ( timer->flags & TIMER_ABSOLUTE )
		timer->value.it_value = timespec_add(timer->value.it_value, timer->value.it_interval);
	else
		timer->value.it_value = timer->value.it_interval;
	Register(timer);
}

} // namespace Sortix
