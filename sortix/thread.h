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

	thread.h
	Describes a thread belonging to a process.

******************************************************************************/

#ifndef SORTIX_THREAD_H
#define SORTIX_THREAD_H

#include "signal.h"

namespace Sortix
{
	class Event;
	class Process;
	class Thread;

	// Adds a thread to the current process.
	Thread* CreateThread(addr_t entry, size_t stacksize = 0);
	Thread* CurrentThread();

	typedef void (*sighandler_t)(int);

	class Thread
	{
	friend Thread* CreateThread(addr_t entry, size_t stacksize);

	public:
		enum State { NONE, RUNNABLE, BLOCKING };

	public:
		static void Init();

	private:
		Thread();
		Thread(const Thread* forkfrom);
		void ResetCallbacks();

	public:
		~Thread();

	public:
		size_t id;
		Process* process;
		pid_t pidbackup;
		addr_t addrspacebackup;
		bool terminated;
		Thread* prevsibling;
		Thread* nextsibling;

	// These are used internally when a thread is waiting for an Event to
	// happen. Consider them private.
	public:
		Event* event;
		Thread* eventnextwaiting;

	// These are some things used internally by the scheduler and should not be
	// touched by anything but it. Consider it private.
	public:
		Thread* schedulerlistprev;
		Thread* schedulerlistnext;
		State state;
		uintmax_t sleepuntil;
		Thread* nextsleepingthread;

	public:
		addr_t stackpos;
		size_t stacksize;
		Signal* currentsignal;
		SignalQueue signalqueue;
		sighandler_t sighandler;

	// After being created/forked, a thread is not inserted into the scheduler.
	// Whenever the thread has been safely established within a process, then
	// call Ready() to finalize the creation and insert it into the scheduler.
	private:
		bool ready;

	public:
		void Ready();

	private:
		CPU::InterruptRegisters registers;

	public:
		Thread* Fork();
		void SaveRegisters(const CPU::InterruptRegisters* src);
		void LoadRegisters(CPU::InterruptRegisters* dest);
		void HandleSignal(CPU::InterruptRegisters* regs);

	private:
		void HandleSignalCPU(CPU::InterruptRegisters* regs);

	public:
		void* scfunc;
		size_t scsize;
		size_t scstate[8];

	public:
		void (*onchildprocessexit)(Thread*, Process*);

	};
}

#endif

