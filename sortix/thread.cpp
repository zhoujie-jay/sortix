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

#include "platform.h"
#include <libmaxsi/memory.h>
#include "process.h"
#include "thread.h"
#include "scheduler.h"
#include "memorymanagement.h"
#include "time.h"

namespace Sortix
{
	Thread::Thread()
	{
		id = 0; // TODO: Make a thread id.
		process = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		sleepuntil = 0;
		nextsleepingthread = 0;
		schedulerlist = NULL;
		schedulerlistprev = NULL;
		schedulerlistnext = NULL;
		state = NONE;
		Maxsi::Memory::Set(&registers, 0, sizeof(registers));
		ready = false;		
	}

	Thread::Thread(const Thread* forkfrom)
	{
		id = forkfrom->id;
		process = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		state = forkfrom->state;
		sleepuntil = forkfrom->sleepuntil;
		Maxsi::Memory::Copy(&registers, &forkfrom->registers, sizeof(registers));
		ready = false;
		stackpos = forkfrom->stackpos;
		stacksize = forkfrom->stacksize;
	}

	Thread::~Thread()
	{
		ASSERT(CurrentProcess() == process);

		Memory::UnmapRangeUser(stackpos, stacksize);
	}

	Thread* Thread::Fork()
	{
		ASSERT(ready);

		Thread* clone = new Thread(this);
		if ( !clone ) { return NULL; }

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
}
