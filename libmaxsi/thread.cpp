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

	thread.cpp
	Exposes system calls for thread creation and management.

******************************************************************************/

#include "platform.h"
#ifdef LIBMAXSI_LIBC
#include <sys/types.h>
#endif
#include "syscall.h"
#include "thread.h"

#ifdef SORTIX_KERNEL
extern "C" void PanicF(const char* Format, ...);
#endif

namespace Maxsi
{
	namespace Thread
	{
		typedef void (*SysEntry)(size_t, Entry, void*);
		DEFN_SYSCALL4(size_t, SysCreate, 0, SysEntry, Entry, const void*, size_t);
		DEFN_SYSCALL1_VOID(SysExit, 1, const void*);
		DEFN_SYSCALL1_VOID(SysSleep, 2, long);
		DEFN_SYSCALL1_VOID(SysUSleep, 3, long);

		void Exit(const void* Result)
		{
			SysExit(Result);
		}

		void ThreadStartup(size_t Id, Entry Start, void* Parameter)
		{
			Exit(Start(Parameter));
		}

		size_t Create(Entry Start, const void* Parameter, size_t StackSize)
		{
			return SysCreate(&ThreadStartup, Start, Parameter, StackSize);
		}

#ifdef LIBMAXSI_LIBC
		extern "C" unsigned sleep(unsigned Seconds) { SysSleep(Seconds); return 0; /* TODO: Posix mentions something about signals and whatnot. */}
#endif

		// TODO: Thinking about it, there is no need for this to be a long.
		// Sortix will never run for that long time, and shouldn't it be unsigned long?
		void Sleep(long Seconds)
		{
			SysSleep(Seconds);			
		}


#ifdef LIBMAXSI_LIBC
		extern "C" int usleep(useconds_t Microseconds) { SysUSleep(Microseconds); return 0; }
#endif

		void USleep(long Microseconds)
		{
			SysUSleep(Microseconds);
		}
	}
}

