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
	Handles the creation and management of threads.

******************************************************************************/

#ifndef SORTIX_SCHEDULER_H
#define SORTIX_SCHEDULER_H

namespace Sortix
{
	class Thread;
	class Process;

	class Process
	{
	public:
		Process(addr_t addrspace);
		~Process();

	private:
		addr_t _addrspace;

	public:
		addr_t GetAddressSpace() { return _addrspace; }

	public:
		bool IsSane() { return _addrspace != 0; }

	};

	class Thread
	{
	public:
		enum State { INFANT, RUNNABLE, UNRUNNABLE, NOOP };
		typedef void* (*Entry)(void* Parameter);

	public:
		Thread(Process* process, size_t id, addr_t stack, size_t stackLength);
		~Thread();

	public:
		size_t GetId() { return _id; }
		Process* GetProcess() { return _process; }

	private:
		size_t _id;
		addr_t _stack;
		size_t _stackLength;
		Process* _process;
		State _state;

	public:
		uintmax_t _sleepMilisecondsLeft;
		Thread* _nextSleepingThread;

	public:
		void Sleep(uintmax_t Miliseconds);

	public:
		Thread* _prevThread;
		Thread* _nextThread;
		Thread** _inThisList;

	public:
		void SaveRegisters(CPU::InterruptRegisters* Src);
		void LoadRegisters(CPU::InterruptRegisters* Dest);

	public:
		CPU::InterruptRegisters _registers;

	private:
		void Relink(Thread** list);
		void Unlink();

	public:
		void SetState(State NewState);
		State GetState();

	private:
		bool _syscall;

	public:
		bool _sysParamsInited;
		size_t _sysParams[16];
	
	public:
		void BeginSyscall(CPU::InterruptRegisters* currentRegisters);
		void SysReturn(size_t result);
		void SysReturnError(size_t result);
		void OnSysReturn();

	};

	namespace Scheduler
	{
		void Init();
		void Switch(CPU::InterruptRegisters* R, uintmax_t TimePassed);
		SORTIX_NORETURN void MainLoop();
		void WakeSleeping(uintmax_t TimePassed);

		// Thread management
		Thread* CreateThread(Process* Process, Thread::Entry Start, void* Parameter1 = NULL, void* Parameter2 = NULL, size_t StackSize = SIZE_MAX);
		void ExitThread(Thread* Thread, void* Result = NULL);
		
		// System Calls.
		void SysCreateThread(CPU::InterruptRegisters* R);
		void SysExitThread(CPU::InterruptRegisters* R);
		void SysSleep(CPU::InterruptRegisters* R);
		void SysUSleep(CPU::InterruptRegisters* R);
	}

	// Scheduling
	Thread* CurrentThread();
	Process* CurrentProcess();
}

#endif

