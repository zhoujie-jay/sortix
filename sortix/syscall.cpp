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

#include <sortix/kernel/platform.h>
#include <libmaxsi/error.h>
#include "syscall.h"
#include <sortix/syscallnum.h>
#include <sortix/kernel/panic.h>
#include "process.h"
#include "thread.h"
#include "scheduler.h"

using namespace Maxsi;

namespace Sortix
{
	namespace Syscall
	{
		extern "C"
		{
			CPU::SyscallRegisters* syscall_state_ptr;
			unsigned system_was_incomplete;
			size_t SYSCALL_MAX;
			volatile void* syscall_list[SYSCALL_MAX_NUM];
		}

		int BadSyscall()
		{
			// TODO: Send signal, set errno, or crash/abort process?
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

		void Incomplete()
		{
			Thread* thread = CurrentThread();

			system_was_incomplete = 1;

			CPU::InterruptRegisters* regs = InterruptRegs();
			thread->SaveRegisters(regs);
			Scheduler::SetThreadState(thread, Thread::State::BLOCKING);
			Scheduler::Switch(regs);
		}

		void Yield()
		{
			Panic("Syscall::Yield() is not implemented because it caused "
			      "instability and other issues.");
		}

		void AsIs()
		{
			system_was_incomplete = 1;
		}

		void ScheduleResumption(Thread* thread)
		{
			Scheduler::SetThreadState(thread, Thread::State::RUNNABLE);
		}

		extern "C" void update_userspace_errno()
		{
			int error = Error::Last();
			if ( !error ) { return; }
			Process* process = CurrentProcess();
			if ( !process->errno ) { return; }
			// TODO: Validate that process->errno is in userspace memory!
			*process->errno = error;
		}

		extern "C" size_t resume_syscall(void* scfunc, size_t scsize, size_t* scstate);

		void Resume(CPU::InterruptRegisters* regs)
		{
			Thread* thread = CurrentThread();

			syscall_state_ptr = (CPU::SyscallRegisters*) regs;

			ASSERT(thread->scfunc);

			size_t* scstate = thread->scstate;
			size_t scsize = thread->scsize;
			void* scfunc = thread->scfunc;

			system_was_incomplete = 0;
			Error::Set(0);

			size_t result = resume_syscall(scfunc, scsize, scstate);

			bool incomplete = (system_was_incomplete);

			system_was_incomplete = 1;

			if ( !incomplete )
			{
				syscall_state_ptr->result = result;
				update_userspace_errno();
				return;
			}

			Incomplete();
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

