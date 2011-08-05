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
#include "scheduler.h"
#include "iirqhandler.h"
#include "isr.h"
#include "globals.h"
#include "iprintable.h"
#include "log.h"
#include "panic.h"

namespace Sortix
{
	namespace Syscall
	{
		void SysStdOutPrint(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			// TODO: Validate our input pointer is legal for the current thread/process!
			R->eax = (uint32_t) Log::Print((const char*) R->ebx);
#else
			#warning "This syscall is not supported on this arch"
			while(true);
#endif
		}

		const size_t NumSyscalls = 5;
		const Syscall Syscalls[NumSyscalls] =
		{
			&Scheduler::SysCreateThread,
			&Scheduler::SysExitThread,
			&Scheduler::SysSleep,
			&Scheduler::SysUSleep,
			&SysStdOutPrint,
		};

		void Init()
		{
			register_interrupt_handler(0x80, &OnCall);
		}

		void OnCall(CPU::InterruptRegisters* registers)
		{
#ifdef PLATFORM_X86
			size_t callId = registers->eax;

			// Make sure the requested syscall exists.
			if ( callId >= NumSyscalls ) { return; }

#ifdef USING_OLD_SYSCALL
			Syscalls[registers->eax](registers);
#else
			Thread* thread = CurrentThread();

			thread->BeginSyscall(registers);

			Syscalls[registers->eax](thread);

			thread->OnSysReturn();
#endif

#else
			#warning "System calls are not available on this platform"
			while(true);
#endif
		}
	}
}

