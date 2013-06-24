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

    unistd/alarm.cpp
    Set an alarm clock for delivery of a signal.

*******************************************************************************/

#include <timespec.h>
#include <unistd.h>

extern "C" unsigned int alarm(unsigned int seconds)
{
	struct timespec delay = timespec_make(seconds, 0);
	struct timespec odelay;
	if ( alarmns(&delay, &odelay) < 0 )
		return 0;
	return odelay.tv_sec + (odelay.tv_nsec ? 1 : 0);
}
