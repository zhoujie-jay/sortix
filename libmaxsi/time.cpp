/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/syscall.h>
#include <errno.h>
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
	}
}
