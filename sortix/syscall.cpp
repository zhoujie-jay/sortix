/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	syscall.h
	Handles system calls from userspace.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include "syscall.h"
#include <sortix/syscallnum.h>
#include <sortix/kernel/panic.h>
#include "process.h"
#include "thread.h"
#include "scheduler.h"

namespace Sortix
{
	namespace Syscall
	{
		extern "C"
		{
			size_t SYSCALL_MAX;
			volatile void* syscall_list[SYSCALL_MAX_NUM];
		}

		int BadSyscall()
		{
			Log::PrintF("I am the bad system call!\n");
			// TODO: Send signal, set errnx	o, or crash/abort process?
			return -1;
		}

		void Init()
		{
			SYSCALL_MAX = SYSCALL_MAX_NUM;
			for ( size_t i = 0; i < SYSCALL_MAX_NUM; i++ )
			{
				syscall_list[i] = (void*) BadSyscall;
			}
		}

		void Register(size_t index, void* funcptr)
		{
			if ( SYSCALL_MAX_NUM <= index )
			{
				PanicF("attempted to register syscall 0x%p to index %zu, but "
				       "SYSCALL_MAX_NYN = %zu", funcptr, index, SYSCALL_MAX_NUM);
			}

			syscall_list[index] = funcptr;
		}
	}
}

