/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	terminal.cpp
	Allows user-space to access terminals.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include <sys/termmode.h>
#include <unistd.h>

namespace Maxsi
{
	DEFN_SYSCALL2(int, SysSetTermMode, SYSCALL_SETTERMMODE, int, unsigned);
	DEFN_SYSCALL2(int, SysGetTermMode, SYSCALL_GETTERMMODE, int, unsigned*);
	DEFN_SYSCALL1(int, SysIsATTY, SYSCALL_ISATTY, int);

	extern "C" int settermmode(int fd, unsigned mode)
	{
		return SysSetTermMode(fd, mode);
	}

	extern "C" int gettermmode(int fd, unsigned* mode)
	{
		return SysGetTermMode(fd, mode);
	}

	extern "C" int isatty(int fd)
	{
		return SysIsATTY(fd);
	}
}
