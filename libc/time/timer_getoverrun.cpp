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

    time/timer_getoverrun.cpp
    Retrieve number of missed events on a timer.

*******************************************************************************/

#include <sys/syscall.h>

#include <time.h>

DEFN_SYSCALL1(int, sys_timer_getoverrun, SYSCALL_TIMER_GETOVERRUN, timer_t);

extern "C" int timer_getoverrun(timer_t timerid)
{
	return sys_timer_getoverrun(timerid);
}
