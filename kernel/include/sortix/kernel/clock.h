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
 * sortix/kernel/clock.h
 * A virtual clock that can be measured and waited upon.
 */

#ifndef INCLUDE_SORTIX_KERNEL_CLOCK_H
#define INCLUDE_SORTIX_KERNEL_CLOCK_H

#include <sys/types.h>

#include <sortix/timespec.h>

#include <sortix/kernel/kthread.h>

namespace Sortix {

class Clock;
class Timer;

class Clock
{
public:
	Clock();
	~Clock();

public:
	Timer* delay_timer;
	Timer* absolute_timer;
	struct timespec current_time;
	struct timespec current_advancement;
	struct timespec resolution;
	kthread_mutex_t clock_mutex;
	bool clock_callable_from_interrupt;
	bool we_disabled_interrupts;

public:
	void SetCallableFromInterrupts(bool callable_from_interrupts);
	void Set(struct timespec* now, struct timespec* res);
	void Get(struct timespec* now, struct timespec* res);
	void Advance(struct timespec duration);
	void Register(Timer* timer);
	void Unlink(Timer* timer);
	void Cancel(Timer* timer);
	void LockClock();
	void UnlockClock();
	struct timespec SleepDelay(struct timespec duration);
	struct timespec SleepUntil(struct timespec expiration);

public: // These should only be called if the clock is locked.
	void RegisterAbsolute(Timer* timer);
	void RegisterDelay(Timer* timer);
	void UnlinkAbsolute(Timer* timer);
	void UnlinkDelay(Timer* timer);

private: // These should only be called if the clock is locked.
	void FireTimer(Timer* timer);
	void TriggerDelay(struct timespec unaccounted);
	void TriggerAbsolute();

};

} // namespace Sortix

#endif
