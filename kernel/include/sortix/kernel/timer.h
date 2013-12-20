/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/timer.h
    A virtual timer that triggers an action in a worker thread when triggered.

*******************************************************************************/

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
