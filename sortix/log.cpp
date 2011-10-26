/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	log.cpp
	A system for logging various messages to the kernel log.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "log.h"
#include "syscall.h"

using namespace Maxsi;

namespace Sortix
{
	namespace Log
	{
		Maxsi::Format::Callback deviceCallback = NULL;
		void* devicePointer = NULL;

		size_t SysPrintString(const char* str)
		{
			// TODO: Check that str is a user-readable string!

			return Print(str);
		}

		void Init(Maxsi::Format::Callback callback, void* user)
		{
			deviceCallback = callback;
			devicePointer = user;

			Syscall::Register(SYSCALL_PRINT_STRING, (void*) SysPrintString);
		}
	}
}
