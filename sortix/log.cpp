/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    log.cpp
    A system for logging various messages to the kernel log.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/log.h>

#include <string.h>

namespace Sortix
{
	namespace Log
	{
		size_t (*deviceCallback)(void*, const char*, size_t) = NULL;
		size_t (*deviceWidth)(void*) = NULL;
		size_t (*deviceHeight)(void*) = NULL;
		bool (*deviceSync)(void*) = NULL;
		void* devicePointer = NULL;

		size_t SysPrintString(const char* str)
		{
			// TODO: Check that str is a user-readable string!

			return Print(str);
		}

		void Init(size_t (*callback)(void*, const char*, size_t),
		          size_t (*widthfunc)(void*),
		          size_t (*heightfunc)(void*),
		          bool (*syncfunc)(void*),
		          void* user)
		{
			deviceCallback = callback;
			deviceWidth = widthfunc;
			deviceHeight = heightfunc;
			deviceSync = syncfunc;
			devicePointer = user;

			Syscall::Register(SYSCALL_PRINT_STRING, (void*) SysPrintString);
		}
	}
}
