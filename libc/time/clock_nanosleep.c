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

    time/clock_nanosleep.c
    Sleep for a duration on a clock.

*******************************************************************************/

#include <sys/syscall.h>

#include <time.h>

DEFN_SYSCALL4(int, sys_clock_nanosleep, SYSCALL_CLOCK_NANOSLEEP, clockid_t,
              int, const struct timespec*, struct timespec*);

int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec* duration, struct timespec* remainder)
{
	return sys_clock_nanosleep(clockid, flags, duration, remainder);
}
