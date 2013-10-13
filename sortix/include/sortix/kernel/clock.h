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

    sortix/kernel/clock.h
    A virtual clock that can be measured and waited upon.

*******************************************************************************/

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
