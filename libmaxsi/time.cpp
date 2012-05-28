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
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/syscall.h>
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

		extern "C" char* ctime(const time_t* /*timep*/)
		{
			return (char*) "ctime(3) is not implemented";
		}
	}
}
