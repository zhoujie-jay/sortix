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

	sortix-vga.cpp
	Provides access to the VGA framebuffer under Sortix. This is highly
	unportable and is very likely to be removed or changed radically.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "sortix-vga.h"

namespace System
{
	namespace VGA
	{
		
		DEFN_SYSCALL0(Frame*, SysCreateFrame, 5);
		DEFN_SYSCALL1(int, SysChangeFrame, 6, int);
		DEFN_SYSCALL1(int, SysDeleteFrame, 7, int);

		Frame* CreateFrame()
		{
			return SysCreateFrame();
		}

		int ChangeFrame(int fd)
		{
			return SysChangeFrame(fd);
		}

		int DeleteFrame(int fd)
		{
			return SysDeleteFrame(fd);
		}
	}
}
