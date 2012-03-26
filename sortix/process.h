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

	process.h
	Describes a process belonging to a subsystem.

******************************************************************************/

#ifndef SORTIX_PROCESS_H
#define SORTIX_PROCESS_H

#include "descriptors.h"
#include "cpu.h"

namespace Sortix
{
	class Thread;
	class Process;
	struct ProcessSegment;

	const size_t DEFAULT_STACK_SIZE = 64*1024;
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
	public:
		Process();
		~Process();

	public:
		static void Init();

	private:
		static pid_t AllocatePID();

	public:
		addr_t addrspace;
		int exitstatus;
		char* workingdir;
		pid_t pid;
		int* errnop;

	public:
		Process* parent;
		Process* prevsibling;
		Process* nextsibling;
		Process* firstchild;
		Process* zombiechild;

	public:
		Thread* firstthread;

	public:
		DescriptorTable descriptors;
		ProcessSegment* segments;

	public:
		bool sigint;

	public:
		int Execute(const char* programname, const byte* program,
		            size_t programsize, int argc, const char* const* argv,
		            int envc, const char* const* envp,
		            CPU::InterruptRegisters* regs);
		void ResetAddressSpace();
		void Exit(int status);

	public:
		bool IsSane() { return addrspace != 0; }

	public:
		Process* Fork();

	private:
		Thread* ForkThreads(Process* processclone);
		void ExecuteCPU(int argc, char** argv, int envc, char** envp,
		                addr_t stackpos, addr_t entry,
		                CPU::InterruptRegisters* regs);

	public:
		void ResetForExecute();

	public:
		inline size_t DefaultStackSize() { return DEFAULT_STACK_SIZE; }

	private:
		addr_t mmapfrom;

	public:
		addr_t AllocVirtualAddr(size_t size);

	public:
		void OnChildProcessExit(Process* process);

	public:
		static Process* Get(pid_t pid);

	private:
		static bool Put(Process* process);
		static void Remove(Process* process);

	};

	Process* CurrentProcess();
}

#endif

