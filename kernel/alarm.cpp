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
 * alarm.cpp
 * Sends a signal after a certain amount of time has passed.
 */

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
