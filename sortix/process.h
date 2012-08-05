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

	process.h
	A named collection of threads.

*******************************************************************************/

#ifndef SORTIX_PROCESS_H
#define SORTIX_PROCESS_H

#include "descriptors.h"
#include "cpu.h"
#include <sortix/kernel/kthread.h>
#include <sortix/fork.h>

namespace Sortix
{
	class Thread;
	class Process;
	struct ProcessSegment;

	const int SEG_NONE = 0;
	const int SEG_TEXT = 1;
	const int SEG_DATA = 2;
	const int SEG_STACK = 3;
	const int SEG_OTHER = 4;

	struct ProcessSegment
	{
	public:
		ProcessSegment() : prev(NULL), next(NULL) { }

	public:
		ProcessSegment* prev;
		ProcessSegment* next;
		addr_t position;
		size_t size;
		int type;

	public:
		bool Intersects(ProcessSegment* segments);
		ProcessSegment* Fork();

	};

	class Process
	{
	friend void Process__OnLastThreadExit(void*);

	public:
		Process();
		~Process();

	public:
		static void Init();

	private:
		static pid_t AllocatePID();

	public:
		addr_t addrspace;
		char* workingdir;
		pid_t pid;

	private:
	// A process may only access its parent if parentlock is locked. A process
	// may only use its list of children if childlock is locked. A process may
	// not access its sibling processes.
		Process* parent;
		Process* prevsibling;
		Process* nextsibling;
		Process* firstchild;
		Process* zombiechild;
		kthread_mutex_t childlock;
		kthread_mutex_t parentlock;
		kthread_cond_t zombiecond;
		size_t zombiewaiting;
		bool iszombie;
		bool nozombify;
		addr_t mmapfrom;
		int exitstatus;

	public:
		Thread* firstthread;
		kthread_mutex_t threadlock;

	public:
		DescriptorTable descriptors;
		ProcessSegment* segments;

	public:
		int Execute(const char* programname, const byte* program,
		            size_t programsize, int argc, const char* const* argv,
		            int envc, const char* const* envp,
		            CPU::InterruptRegisters* regs);
		void ResetAddressSpace();
		void Exit(int status);
		pid_t Wait(pid_t pid, int* status, int options);
		bool DeliverSignal(int signum);
		void OnThreadDestruction(Thread* thread);
		int GetParentProcessId();
		void AddChildProcess(Process* child);
		void ScheduleDeath();
		void AbortConstruction();

	public:
		Process* Fork();

	private:
		void ExecuteCPU(int argc, char** argv, int envc, char** envp,
		                addr_t stackpos, addr_t entry,
		                CPU::InterruptRegisters* regs);
		void OnLastThreadExit();
		void LastPrayer();
		void NotifyChildExit(Process* child, bool zombify);
		void NotifyNewZombies();

	public:
		void ResetForExecute();
		addr_t AllocVirtualAddr(size_t size);

	public:
		static Process* Get(pid_t pid);
		static pid_t HackGetForegroundProcess();

	private:
		static bool Put(Process* process);
		static void Remove(Process* process);

	};

	void InitializeThreadRegisters(CPU::InterruptRegisters* regs,
                                   const tforkregs_t* requested);
	Process* CurrentProcess();
}

#endif

