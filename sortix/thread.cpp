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

	thread.cpp
	Describes a thread belonging to a process.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "event.h"
#include "process.h"
#include "thread.h"
#include "scheduler.h"
#include <sortix/kernel/memorymanagement.h>
#include "time.h"
#include "syscall.h"

using namespace Maxsi;

namespace Sortix
{
	Thread::Thread()
	{
		id = 0; // TODO: Make a thread id.
		process = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		event = NULL;
		eventnextwaiting = NULL;
		sleepuntil = 0;
		nextsleepingthread = NULL;
		schedulerlistprev = NULL;
		schedulerlistnext = NULL;
		state = NONE;
		Maxsi::Memory::Set(&registers, 0, sizeof(registers));
		ready = false;
		scfunc = NULL;
		currentsignal = NULL;
		sighandler = NULL;
		pidbackup = -1;
		addrspacebackup = 0UL;
		terminated = false;
		ResetCallbacks();
	}

	Thread::Thread(const Thread* forkfrom)
	{
		id = forkfrom->id;
		process = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		state = forkfrom->state;
		event = NULL;
		eventnextwaiting = NULL;
		sleepuntil = forkfrom->sleepuntil;
		Maxsi::Memory::Copy(&registers, &forkfrom->registers, sizeof(registers));
		ready = false;
		stackpos = forkfrom->stackpos;
		stacksize = forkfrom->stacksize;
		nextsleepingthread = NULL;
		schedulerlistprev = NULL;
		schedulerlistnext = NULL;
		scfunc = NULL;
		sighandler = forkfrom->sighandler;
		pidbackup = -1;
		addrspacebackup = 0UL;
		terminated = false;
		ResetCallbacks();
	}

	void Thread::ResetCallbacks()
	{
		onchildprocessexit = NULL;
	}

	Thread::~Thread()
	{
		ASSERT(CurrentProcess() == process);
		ASSERT(nextsleepingthread == NULL);

		if ( event ) { event->Unregister(this); }

		// Delete information about signals being processed.
		while ( currentsignal )
		{
			Signal* todelete = currentsignal;
			currentsignal = currentsignal->nextsignal;
			delete todelete;
		}

		Memory::UnmapRangeUser(stackpos, stacksize);

		terminated = true;
	}

	Thread* Thread::Fork()
	{
		ASSERT(ready);

		Signal* clonesignal = NULL;
		if ( currentsignal )
		{
			clonesignal = currentsignal->Fork();
			if ( !clonesignal ) { return NULL; }
		}

		Thread* clone = new Thread(this);
		if ( !clone )
		{
			while ( clonesignal )
			{
				Signal* todelete = clonesignal;
				clonesignal = clonesignal->nextsignal;
				delete todelete;
			}
			return NULL;
		}

		clone->currentsignal = clonesignal;

		return clone;
	}

	void CreateThreadCPU(Thread* thread, addr_t entry);

	Thread* CreateThread(addr_t entry, size_t stacksize)
	{
		Process* process = CurrentProcess();

		if ( stacksize == 0 ) { stacksize = process->DefaultStackSize(); }

		// TODO: Find some unused virtual address space of the needed size
		// somewhere in the current process.
		addr_t stackpos = process->AllocVirtualAddr(stacksize);
		if ( !stackpos ) { return NULL; }

		if ( !Memory::MapRangeUser(stackpos, stacksize) )
		{
			// TODO: Free the reserved virtual memory area.
			return NULL;
		}

		Thread* thread = new Thread();
		if ( !thread )
		{
			Memory::UnmapRangeUser(stackpos, stacksize);
			// TODO: Free the reserved virtual memory area.
			return NULL;
		}

		thread->stackpos = stackpos;
		thread->stacksize = stacksize;

		// Set up the thread state registers.
		CreateThreadCPU(thread, entry);

		// Create the family tree.
		thread->process = process;
		Thread* firsty = process->firstthread;
		if ( firsty ) { firsty->prevsibling = thread; }
		thread->nextsibling = firsty;
		process->firstthread = thread;

		thread->Ready();

		Scheduler::SetThreadState(thread, Thread::State::RUNNABLE);

		return thread;
	}

	void Thread::Ready()
	{
		if ( ready ) { return; }
		ready = true;

		this->pidbackup = process->pid;
		this->addrspacebackup = process->addrspace;

		if ( Time::MicrosecondsSinceBoot() < sleepuntil )
		{
			uintmax_t howlong = sleepuntil - Time::MicrosecondsSinceBoot();
			Scheduler::PutThreadToSleep(this, howlong);
		}
		else if ( state == State::RUNNABLE )
		{
			state = State::NONE; // Since we are in no linked list.
			Scheduler::SetThreadState(this, State::RUNNABLE);
		}
	}

	void Thread::HandleSignal(CPU::InterruptRegisters* regs)
	{
		Signal* override = signalqueue.Pop(currentsignal);
		if ( !override ) { return; }
		if ( !sighandler ) { delete override; return; }

		if ( override->signum == SIGKILL )
		{

		}


		override->nextsignal = currentsignal;
		Maxsi::Memory::Copy(&override->regs, regs, sizeof(override->regs));
		currentsignal = override;

		HandleSignalCPU(regs);
	}

	void SysSigReturn()
	{
		Thread* thread = CurrentThread();
		if ( !thread->currentsignal ) { return; }

		CPU::InterruptRegisters* dest = Syscall::InterruptRegs();
		CPU::InterruptRegisters* src = &thread->currentsignal->regs;

		Maxsi::Memory::Copy(dest, src, sizeof(CPU::InterruptRegisters));
		thread->currentsignal = thread->currentsignal->nextsignal;
		Syscall::AsIs();
	}

	void SysRegisterSignalHandler(sighandler_t sighandler)
	{
		CurrentThread()->sighandler = sighandler;
	}

	int SysKill(pid_t pid, int signum)
	{
		if ( signum < 0 || 128 <= signum ) { Error::Set(EINVAL); return -1; }

		// Protect the system idle process.
		if ( pid == 0 ) { Error::Set(EPERM); return -1; }

		Process* process = Process::Get(pid);
		if ( !process ) { Error::Set(ESRCH); return -1; }

		// TODO: Protect init?
		// TODO: Check for permission.
		// TODO: Check for zombies.

		Thread* currentthread = CurrentThread();
		Thread* thread = NULL;
		if ( currentthread->process->pid == pid ) { thread = currentthread; }
		if ( !thread ) { thread = process->firstthread; }
		if ( !thread ) { Error::Set(ESRCH); return -1; /* TODO: Zombie? */ }

		// TODO: If thread is not runnable, wake it and runs its handler?
		if ( !thread->signalqueue.Push(signum) )
		{
			// TODO: Possibly kill the process?
		}

		if ( thread == currentthread )
		{
			Syscall::SyscallRegs()->result = 0;
			thread->HandleSignal(Syscall::InterruptRegs());
		}

		return 0;
	}

	void Thread::Init()
	{
		Syscall::Register(SYSCALL_KILL, (void*) SysKill);
		Syscall::Register(SYSCALL_REGISTER_SIGNAL_HANDLER, (void*) SysRegisterSignalHandler);
		Syscall::Register(SYSCALL_SIGRETURN, (void*) SysSigReturn);
	}
}
