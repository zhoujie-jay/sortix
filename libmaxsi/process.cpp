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

	process.cpp
	Exposes system calls for process creation and management.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "process.h"

namespace Maxsi
{
	namespace Process
	{
		DEFN_SYSCALL3(int, SysExecute, 10, const char*, int, const char**);
		DEFN_SYSCALL0_VOID(SysPrintPathFiles, 11);

		int Execute(const char* filepath, int argc, const char** argv)
		{
			return SysExecute(filepath, argc, argv);
		}

		void PrintPathFiles()
		{
			SysPrintPathFiles();
		}
	}
}

