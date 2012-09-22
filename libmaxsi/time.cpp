/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	time.cpp
	Useful time functions.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

namespace Maxsi
{
	namespace Time
	{
		DEFN_SYSCALL1(int, SysUptime, SYSCALL_UPTIME, uintmax_t*);

		extern "C" int uptime(uintmax_t* usecssinceboot)
		{
			return SysUptime(usecssinceboot);
		}

		extern "C" clock_t clock(void)
		{
			errno = ENOTSUP;
			return -1;
		}

		extern "C" int gettimeofday(struct timeval* tp, void* /*tzp*/)
		{
			uintmax_t sinceboot;
			uptime(&sinceboot);
			tp->tv_sec = sinceboot / 1000000ULL;
			tp->tv_usec = sinceboot % 1000000ULL;
			return 0;
		}

		extern "C" time_t time(time_t* t)
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			time_t result = tv.tv_sec;
			return t ? *t = result : result;
		}

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

		extern "C" struct tm* gmtime_r(const time_t* timer, struct tm* ret)
		{
			return localtime_r(timer, ret);
		}

		// TODO: Who the hell designed the below two functions? I vote that
		// these be removed from Sortix soon - although that'd require fixing
		// a lot of applications..

		extern "C" struct tm* localtime(const time_t* timer)
		{
			static struct tm hack_localtime_ret;
			return localtime_r(timer, &hack_localtime_ret);
		}

		extern "C" struct tm* gmtime(const time_t* timer)
		{
			static struct tm hack_gmtime_ret;
			return gmtime_r(timer, &hack_gmtime_ret);
		}

		extern "C" char* ctime(const time_t* /*timep*/)
		{
			return (char*) "ctime(3) is not implemented";
		}

		extern "C" int utime(const char* filepath, const struct utimbuf* times)
		{
			// TODO: Sure, I did that! There is no kernel support for this yet.
			return 0;
		}
	}
}
