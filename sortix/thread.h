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

	thread.h
	Describes a thread belonging to a process.

*******************************************************************************/

#ifndef SORTIX_THREAD_H
#define SORTIX_THREAD_H

#include <sortix/signal.h>
#include "signal.h"

typedef struct multiboot_info multiboot_info_t;

namespace Sortix
{
	class Process;
	class Thread;

	extern "C" void KernelInit(unsigned long magic, multiboot_info_t* bootinfo);

	typedef void (*ThreadEntry)(void* user);

	// Simply exits the kernel thread.
	void KernelThreadExit() SORTIX_NORETURN;

	// Internally used as a kernel thread entry point that exits the thread
	// upon the actual thread entry returning.
	extern "C" void BootstrapKernelThread(void* user, ThreadEntry entry) SORTIX_NORETURN;

	// These functions create a new kernel process but doesn't start it.
	Thread* CreateKernelThread(Process* process, CPU::InterruptRegisters* regs);
	Thread* CreateKernelThread(Process* process, ThreadEntry entry, void* user,
	                           size_t stacksize = 0);
	Thread* CreateKernelThread(ThreadEntry entry, void* user, size_t stacksize = 0);

	// This function can be used to start a thread from the above functions.
	void StartKernelThread(Thread* thread);

	// Alternatively, these functions both create and start the thread.
	Thread* RunKernelThread(Process* process, CPU::InterruptRegisters* regs);
	Thread* RunKernelThread(Process* process, ThreadEntry entry, void* user,
	                        size_t stacksize = 0);
	Thread* RunKernelThread(ThreadEntry entry, void* user, size_t stacksize = 0);

	void SetupKernelThreadRegs(CPU::InterruptRegisters* regs, ThreadEntry entry,
	                           void* user, addr_t stack, size_t stacksize);

	extern "C" void Thread__OnSigKill(Thread* thread);

	typedef void (*sighandler_t)(int);

	class Thread
	{
	friend Thread* CreateKernelThread(Process* process,
	                                  CPU::InterruptRegisters* regs);
	friend void KernelInit(unsigned long magic, multiboot_info_t* bootinfo);
	friend void Thread__OnSigKill(Thread* thread);

	public:
		enum State { NONE, RUNNABLE, BLOCKING, DEAD };

	public:
		static void Init();

	private:
		Thread();

	public:
		~Thread();

	public:
		size_t id;
		Process* process;
		bool terminated;
		Thread* prevsibling;
		Thread* nextsibling;

	// These are some things used internally by the scheduler and should not be
	// touched by anything but it. Consider it private.
	public:
		Thread* schedulerlistprev;
		Thread* schedulerlistnext;
		volatile State state;

	public:
		addr_t addrspace;
		addr_t stackpos;
		size_t stacksize;
		sighandler_t sighandler;
		addr_t kernelstackpos;
		size_t kernelstacksize;
		bool kernelstackmalloced;

	private:
		CPU::InterruptRegisters registers;
		Signal::Queue signalqueue;
		int currentsignal;
		int siglevel;
		int signums[SIG_NUM_LEVELS];
		CPU::InterruptRegisters sigregs[SIG_NUM_LEVELS];

	public:
		void SaveRegisters(const CPU::InterruptRegisters* src);
		void LoadRegisters(CPU::InterruptRegisters* dest);
		void HandleSignal(CPU::InterruptRegisters* regs);
		void HandleSigreturn(CPU::InterruptRegisters* regs);
		bool DeliverSignal(int signum);
		addr_t SwitchAddressSpace(addr_t newaddrspace);

	private:
		void GotoOnSigKill(CPU::InterruptRegisters* regs);
		void OnSigKill() SORTIX_NORETURN;
		void LastPrayer();
		void SetHavePendingSignals();
		void HandleSignalFixupRegsCPU(CPU::InterruptRegisters* regs);
		void HandleSignalCPU(CPU::InterruptRegisters* regs);

	};

	Thread* CurrentThread();
}

#endif

