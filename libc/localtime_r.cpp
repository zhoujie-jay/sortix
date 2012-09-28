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

	localtime_r.cpp
	Transform date and time.

*******************************************************************************/

#include <time.h>

extern "C" struct tm* localtime_r(const time_t* timer, struct tm* ret)
{
	time_t time = *timer;
	ret->tm_sec = time % 60; // No leap seconds.
	ret->tm_min = (time / 60) % 60;
	ret->tm_hour = (time / 60 / 60) % 24;
	ret->tm_mday = 0;
	ret->tm_mon = 0;
	ret->tm_year = 0;
	ret->tm_wday = 0;
	ret->tm_isdst = 0;
	return ret;
}
