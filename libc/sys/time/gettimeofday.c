/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sys/time/gettimeofday.c
    Get date and time.

*******************************************************************************/

#include <sys/time.h>

#include <time.h>

int gettimeofday(struct timeval* tp, void* tzp)
{
	(void) tzp;
	struct timespec now;
	if ( clock_gettime(CLOCK_REALTIME, &now) < 0 )
		return -1;
	tp->tv_sec = now.tv_sec;
	tp->tv_usec = now.tv_nsec / 1000;
	return 0;
}
