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

    time/asctime.cpp
    Convert date and time to a string.

*******************************************************************************/

#include <assert.h>
#include <time.h>
#include <stdio.h>

// TODO: Oh god. This function is horrible! It's obsolescent in POSIX - but it's
//       still widely used and I need it at the moment to port software. Please
//       do remove this function when all the calls to it has been purged.
extern "C" char* asctime(const struct tm* tm)
{
	static char buf[26];
	return asctime_r(tm, buf);
}
