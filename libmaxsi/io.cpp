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

	io.h
	Functions for management of input and output.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "io.h"

namespace Maxsi
{
	namespace StdOut
	{
		DEFN_SYSCALL1(size_t, SysPrint, 4, const char*);

		size_t Print(const char* Message)
		{
			return SysPrint(Message);
		}

#ifdef LIBMAXSI_LIBC
		extern "C" int printf(const char* /*restrict*/ format, ...)
		{
			// TODO: The format string is currently being ignored!
			return Print(format);
		}
#endif
	}
}
