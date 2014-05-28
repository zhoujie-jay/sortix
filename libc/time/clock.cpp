/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013

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

    time/clock.cpp
    Get process execution time.

*******************************************************************************/

#include <assert.h>
#include <time.h>

// TODO: This function is crap and has been replaced by clock_gettime.
extern "C" clock_t clock()
{
	struct timespec now;
	if ( clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now) < 0 )
		return -1;
	static_assert(CLOCKS_PER_SEC == 1000000, "CLOCKS_PER_SEC == 1000000");
	return (clock_t) now.tv_sec * 1000000 + (clock_t) now.tv_sec / 1000;
}
