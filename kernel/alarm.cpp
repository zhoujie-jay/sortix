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

    alarm.cpp
    Sends a signal after a certain amount of time has passed.

*******************************************************************************/

#include <errno.h>
#include <timespec.h>

#include <sortix/itimerspec.h>
#include <sortix/signal.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/timer.h>

namespace Sortix {

static void alarm_handler(Clock* /*clock*/, Timer* /*timer*/, void* user)
{
	Process* process = (Process*) user;
	ScopedLock lock(&process->user_timers_lock);
	process->DeliverSignal(SIGALRM);
}

int sys_alarmns(const struct timespec* user_delay, struct timespec* user_odelay)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->user_timers_lock);

	struct itimerspec delay, odelay;
	if ( !CopyFromUser(&delay.it_value, user_delay, sizeof(*user_delay)) )
		return -1;
	delay.it_interval = timespec_nul();

	process->alarm_timer.Set(&delay, &odelay, 0, alarm_handler, (void*) process);

	if ( !CopyToUser(user_odelay, &odelay.it_value, sizeof(*user_odelay)) )
		return -1;

	return 0;
}

} // namespace Sortix
