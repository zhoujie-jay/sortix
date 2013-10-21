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

    time/asctime_r.cpp
    Convert date and time to a string.

*******************************************************************************/

#include <assert.h>
#include <time.h>
#include <stdio.h>

// TODO: Oh god. This function is horrible! It's obsolescent in POSIX - but it's
//       still widely used and I need it at the moment to port software. Please
//       do remove this function when all the calls to it has been purged.
extern "C" char* asctime_r(const struct tm* tm, char* buf)
{
	static char weekday_names[7][4] =
		{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static char month_names[12][4] =
		{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
		  "Nov", "Dec" };
	sprintf(buf, "%.3s %.3s%3d %.2d:%.2d%.2d %d\n",
	             weekday_names[tm->tm_wday],
	             month_names[tm->tm_mon],
	             tm->tm_mday,
	             tm->tm_hour,
	             tm->tm_min,
	             tm->tm_sec,
	             tm->tm_year + 1900);
	return buf;
}
