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

	scheduler.h
	Handles context switching between tasks and deciding when to execute what.

******************************************************************************/

#ifndef SORTIX_SCHEDULER_H
#define SORTIX_SCHEDULER_H

#include "thread.h"

namespace Sortix
{
	namespace Scheduler
	{
		void Init();
		void MainLoop() SORTIX_NORETURN;
		void Switch(CPU::InterruptRegisters* regs);
		void SetIdleThread(Thread* thread);
		void SetDummyThreadOwner(Process* process);
		void SetInitProcess(Process* init);
		Process* GetInitProcess();

		void SetThreadState(Thread* thread, Thread::State state);
		Thread::State GetThreadState(Thread* thread);
		void PutThreadToSleep(Thread* thread, uintmax_t usecs);

		void SigIntHack();
	}
}

#endif

