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

	scheduler.cpp
	Handles context switching between tasks and deciding when to execute what.

******************************************************************************/

#include "platform.h"
#include "panic.h"
#include "thread.h"
#include "process.h"
#include "time.h"
#include "scheduler.h"
#include "memorymanagement.h"
#include "syscall.h"
#include "sound.h" // HACK FOR SIGINT

namespace Sortix
{
	void SysExit(int status); // HACK FOR SIGINT

	// Internal forward-declarations.
	namespace Scheduler
	{
		void InitCPU();
		Thread* PopNextThread();
		void WakeSleeping();
		void LogBeginContextSwitch(Thread* current, const CPU::InterruptRegisters* state);
		void LogContextSwitch(Thread* current, Thread* next);
		void LogEndContextSwitch(Thread* current, const CPU::InterruptRegisters* state);
		void SysSleep(size_t secs);
		void SysUSleep(size_t usecs);
		void HandleSigIntHack(CPU::InterruptRegisters* regs);
	}

	namespace Scheduler
	{
		byte dummythreaddata[sizeof(Thread)];
		Thread* dummythread = (Thread*) &dummythreaddata;
		Thread* currentthread;
		Thread* idlethread;
		Thread* firstrunnablethread;
		Thread* firstsleepingthread;
		Process* initprocess;
		bool hacksigintpending = false;

		void Init()
		{
			// We use a dummy so that the first context switch won't crash when
			// currentthread is accessed. This lets us avoid checking whether
			// currentthread is NULL (which it only will be once) which gives
			// simpler code.
			currentthread = dummythread;
			firstrunnablethread = NULL;
			idlethread = NULL;
			hacksigintpending = false;

			Syscall::Register(SYSCALL_SLEEP, (void*) SysSleep);
			Syscall::Register(SYSCALL_USLEEP, (void*) SysUSleep);

			InitCPU();
		}

		// The no operating thread is a thread stuck in an infinite loop that
		// executes absolutely nothing, which is only run when the system has
		// nothing to do.
		void SetIdleThread(Thread* thread)
		{
			ASSERT(idlethread == NULL);
			idlethread = thread;
			SetThreadState(thread, Thread::State::NONE);
		}

		void SetDummyThreadOwner(Process* process)
		{
			dummythread->process = process;
		}

		void SetInitProcess(Process* init)
		{
			initprocess = init;
		}

		Process* GetInitProcess()
		{
			return initprocess;
		}

		void MainLoop()
		{
			// Wait for the first hardware interrupt to trigger a context switch
			// into the first task! Then the init process should gracefully
			// start executing.
			while(true);
		}

		void Switch(CPU::InterruptRegisters* regs)
		{
			LogBeginContextSwitch(currentthread, regs);

			if ( hacksigintpending ) { HandleSigIntHack(regs); }

			WakeSleeping();

			Thread* nextthread = PopNextThread();
			if ( !nextthread ) { Panic("had no thread to switch to"); }

			LogContextSwitch(currentthread, nextthread);
			if ( nextthread == currentthread ) { return; }

			currentthread->SaveRegisters(regs);
			nextthread->LoadRegisters(regs);

			addr_t newaddrspace = nextthread->process->addrspace;
			Memory::SwitchAddressSpace(newaddrspace);
			currentthread = nextthread;

			nextthread->HandleSignal(regs);

			LogEndContextSwitch(currentthread, regs);

			if ( currentthread->scfunc ) { Syscall::Resume(regs); }
		}

		void ProcessTerminated(CPU::InterruptRegisters* regs)
		{
			currentthread = dummythread;
			Switch(regs);
		}

		const bool DEBUG_BEGINCTXSWITCH = false;
		const bool DEBUG_CTXSWITCH = false;
		const bool DEBUG_ENDCTXSWITCH = false;

		void LogBeginContextSwitch(Thread* current, const CPU::InterruptRegisters* state)
		{
			if ( DEBUG_BEGINCTXSWITCH && current->process->pid != 0 )
			{
				Log::PrintF("Switching from 0x%p", current);
				state->LogRegisters();
				Log::Print("\n");
			}
		}

		void LogContextSwitch(Thread* current, Thread* next)
		{
			if ( DEBUG_CTXSWITCH && current != next )
			{
				Log::PrintF("switching from %u:%u (0x%p) to %u:%u (0x%p) \n",
				            current->process->pid, 0, current,
				            next->process->pid, 0, next);
			}
		}

		void LogEndContextSwitch(Thread* current, const CPU::InterruptRegisters* state)
		{
			if ( DEBUG_ENDCTXSWITCH && current->process->pid != 0 )
			{
				Log::PrintF("Switched to 0x%p", current);
				state->LogRegisters();
				Log::Print("\n");
			}
		}

		Thread* PopNextThread()
		{
			if ( !firstrunnablethread ) { return idlethread; }
			Thread* result = firstrunnablethread;
			firstrunnablethread = firstrunnablethread->schedulerlistnext;
			return result;
		}

		void SetThreadState(Thread* thread, Thread::State state)
		{
			if ( thread->state == state ) { return; }

			if ( thread->state == Thread::State::RUNNABLE )
			{
				if ( thread == firstrunnablethread ) { firstrunnablethread = thread->schedulerlistnext; }
				if ( thread == firstrunnablethread ) { firstrunnablethread = NULL; }
				thread->schedulerlistprev->schedulerlistnext = thread->schedulerlistnext;
				thread->schedulerlistnext->schedulerlistprev = thread->schedulerlistprev;
				thread->schedulerlistprev = NULL;
				thread->schedulerlistnext = NULL;
			}

			// Insert the thread into the scheduler's carousel linked list.
			if ( state == Thread::State::RUNNABLE )
			{
				if ( firstrunnablethread == NULL ) { firstrunnablethread = thread; }
				thread->schedulerlistprev = firstrunnablethread->schedulerlistprev;
				thread->schedulerlistnext = firstrunnablethread;
				firstrunnablethread->schedulerlistprev = thread;
				thread->schedulerlistprev->schedulerlistnext = thread;
			}

			thread->state = state;
		}

		Thread::State GetThreadState(Thread* thread)
		{
			return thread->state;
		}

		void PutThreadToSleep(Thread* thread, uintmax_t usecs)
		{
			SetThreadState(thread, Thread::State::BLOCKING);
			thread->sleepuntil = Time::MicrosecondsSinceBoot() + usecs;

			// We use a simple linked linked list sorted after wake-up time to
			// keep track of the threads that are sleeping.

			if ( firstsleepingthread == NULL )
			{
				thread->nextsleepingthread = NULL;
				firstsleepingthread = thread;
				return;
			}

			if ( thread->sleepuntil < firstsleepingthread->sleepuntil )
			{
				thread->nextsleepingthread = firstsleepingthread;
				firstsleepingthread = thread;
				return;
			}

			for ( Thread* tmp = firstsleepingthread; tmp != NULL; tmp = tmp->nextsleepingthread )
			{
				if ( tmp->nextsleepingthread == NULL ||
					 thread->sleepuntil < tmp->nextsleepingthread->sleepuntil )
				{
					thread->nextsleepingthread = tmp->nextsleepingthread;
					tmp->nextsleepingthread = thread;
					return;
				}
			}
		}

		void EarlyWakeUp(Thread* thread)
		{
			uintmax_t now = Time::MicrosecondsSinceBoot();
			if ( thread->sleepuntil < now ) { return; }
			thread->sleepuntil = now;

			SetThreadState(thread, Thread::State::RUNNABLE);

			if ( firstsleepingthread == thread )
			{
				firstsleepingthread = thread->nextsleepingthread;
				thread->nextsleepingthread = NULL;
				return;
			}

			for ( Thread* tmp = firstsleepingthread; tmp->nextsleepingthread != NULL; tmp = tmp->nextsleepingthread )
			{
				if ( tmp->nextsleepingthread == thread )
				{
					tmp->nextsleepingthread = thread->nextsleepingthread;
					thread->nextsleepingthread = NULL;
					return;
				}
			}
		}

		void WakeSleeping()
		{
			uintmax_t now = Time::MicrosecondsSinceBoot();

			while ( firstsleepingthread && firstsleepingthread->sleepuntil < now )
			{
				SetThreadState(firstsleepingthread, Thread::State::RUNNABLE);
				Thread* next = firstsleepingthread->nextsleepingthread;
				firstsleepingthread->nextsleepingthread = NULL;
				firstsleepingthread = next;
			}
		}

		void HandleSigIntHack(CPU::InterruptRegisters* regs)
		{
			if ( currentthread == idlethread ) { return; }

			hacksigintpending = false;

			// HACK: Don't crash init or sh.
			Process* process = CurrentProcess();
			if ( process->pid < 3 ) { return; }

			Sound::Mute();
			Log::PrintF("^C\n");

			process->Exit(130);
			currentthread = dummythread;
		}

		void SigIntHack()
		{
			hacksigintpending = true;
		}

		void SysSleep(size_t secs)
		{
			Thread* thread = currentthread;
			uintmax_t timetosleep = ((uintmax_t) secs) * 1000ULL * 1000ULL;
			if ( timetosleep == 0 ) { return; }
			PutThreadToSleep(thread, timetosleep);
			Syscall::Incomplete();
		}

		void SysUSleep(size_t usecs)
		{
			Thread* thread = currentthread;
			uintmax_t timetosleep = usecs;
			if ( timetosleep == 0 ) { return; }
			PutThreadToSleep(thread, timetosleep);
			Syscall::Incomplete();
		}
	}

	Thread* CurrentThread()
	{
		return Scheduler::currentthread;
	}

	Process* CurrentProcess()
	{
		return Scheduler::currentthread->process;
	}
}
