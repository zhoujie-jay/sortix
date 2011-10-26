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

	syscall.h
	Handles system calls from userspace safely.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "syscallnum.h"
#include "panic.h"
#include "thread.h"
#include "scheduler.h"

#include "log.h" // DEBUG

namespace Sortix
{
	namespace Syscall
	{
		extern "C"
		{
			CPU::SyscallRegisters* syscall_state_ptr;
			int system_was_incomplete;
			size_t SYSCALL_MAX = SYSCALL_MAX_NUM;
			void* syscall_list[SYSCALL_MAX_NUM];
		}

		int BadSyscall()
		{
			// TODO: Send signal, set errno, or crash/abort process?
			return -1;
		}

		void Init()
		{
			for ( size_t i = 0; i < SYSCALL_MAX; i++ )
			{
				syscall_list[i] = (void*) BadSyscall;
			}
		}

		void Register(size_t index, void* funcptr)
		{
			if ( SYSCALL_MAX <= index )
			{
				PanicF("attempted to register syscall 0x%p to index %zu, but "
				       "SYSCALL_MAX = %zu", funcptr, index, SYSCALL_MAX);
			}

			syscall_list[index] = funcptr; 
		}

		void Incomplete()
		{
			system_was_incomplete = 1;

			CPU::InterruptRegisters* regs = InterruptRegs();
			CurrentThread()->SaveRegisters(regs);
			// TODO: Make the thread blocking if not already.
			Scheduler::Switch(regs, 0);
		}

		CPU::InterruptRegisters* InterruptRegs()
		{
			return (CPU::InterruptRegisters*) syscall_state_ptr;
		}

		CPU::SyscallRegisters* SyscallRegs()
		{
			return syscall_state_ptr;
		}
	}
}

