/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    time/timer_settime.c
    Set time remaining on a timer.

*******************************************************************************/

#include <sys/syscall.h>

#include <time.h>

DEFN_SYSCALL4(int, sys_timer_settime, SYSCALL_TIMER_SETTIME, timer_t, int,
              const struct itimerspec*, struct itimerspec*);

int timer_settime(timer_t timerid,
                  int flags,
                  const struct itimerspec* restrict value,
                  struct itimerspec* restrict ovalue)
{
	return sys_timer_settime(timerid, flags, value, ovalue);
}
