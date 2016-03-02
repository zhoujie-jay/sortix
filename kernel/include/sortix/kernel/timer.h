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
 * sortix/kernel/timer.h
 * A virtual timer that triggers an action in a worker thread when triggered.
 */

#ifndef INCLUDE_SORTIX_KERNEL_TIMER_H
#define INCLUDE_SORTIX_KERNEL_TIMER_H

#include <sortix/timespec.h>
#include <sortix/itimerspec.h>

#include <sortix/kernel/kthread.h>

namespace Sortix {

class Clock;
class Timer;

const int TIMER_ABSOLUTE = 1 << 0;
const int TIMER_ACTIVE = 1 << 1;
const int TIMER_FIRING = 1 << 2;
const int TIMER_FUNC_INTERRUPT_HANDLER = 1 << 3;
const int TIMER_FUNC_ADVANCE_THREAD = 1 << 4;

class Timer
{
public:
	Timer();
	~Timer();

public:
	struct itimerspec value;
	Clock* clock;
	Timer* prev_timer;
	Timer* next_timer;
	void (*callback)(Clock* clock, Timer* timer, void* user);
	void* user;
	size_t num_firings_scheduled;
	size_t num_overrun_events;
	int flags;

private:
	void Fire();
	void GetInternal(struct itimerspec* current);

public:
	void Attach(Clock* the_clock);
	void Detach();
	bool IsAttached() const { return clock; }
	void Cancel();
	Clock* GetClock() const { return clock; }
	void Get(struct itimerspec* current);
	void Set(struct itimerspec* value, struct itimerspec* ovalue, int flags,
	         void (*callback)(Clock*, Timer*, void*), void* user);

};

} // namespace Sortix

#endif
