/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	time.h
	Time declarations.

*******************************************************************************/

#ifndef	_TIME_H
#define	_TIME_H 1

#include <features.h>

__BEGIN_DECLS

@include(clock_t.h)
@include(time_t.h)

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_isdst;
};

struct utimbuf
{
	time_t actime;
	time_t modtime;
};

clock_t clock(void);
time_t time(time_t* t);
char* ctime(const time_t* timep);
struct tm* localtime_r(const time_t* timer, struct tm* ret);
struct tm* gmtime_r(const time_t* timer, struct tm* ret);
#if !defined(_SORTIX_SOURCE)
struct tm* localtime(const time_t* timer);
struct tm* gmtime(const time_t* timer);
#endif
int utime(const char* filepath, const struct utimbuf* times);

__END_DECLS

#endif
