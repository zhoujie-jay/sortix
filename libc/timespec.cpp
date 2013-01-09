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

    timespec.cpp
    Utility functions for manipulation of struct timespec.

*******************************************************************************/

#include <timespec.h>

static const long NANOSECONDS_PER_SECOND = 1000000000L;

// TODO: The C modulo operator doesn't do exactly what we desire.
static long proper_modulo(long a, long b)
{
	if ( 0 <= a )
		return a % b;
	long tmp = - (unsigned long) a % b;
	return tmp ? tmp : 0;
}

extern "C" struct timespec timespec_canonalize(struct timespec t)
{
	t.tv_sec += t.tv_nsec / NANOSECONDS_PER_SECOND;
	t.tv_nsec = proper_modulo(t.tv_nsec, NANOSECONDS_PER_SECOND);
	return t;
}

extern "C" struct timespec timespec_add(struct timespec a, struct timespec b)
{
	struct timespec ret;
	a = timespec_canonalize(a);
	b = timespec_canonalize(b);
	ret.tv_sec = a.tv_sec + b.tv_sec;
	ret.tv_nsec = a.tv_nsec + b.tv_nsec;
	return timespec_canonalize(ret);
}

extern "C" struct timespec timespec_sub(struct timespec a, struct timespec b)
{
	a = timespec_canonalize(a);
	b = timespec_canonalize(b);
	return timespec_add(a, timespec_neg(b));
}
