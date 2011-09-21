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

	process.cpp
	Describes a process belonging to a subsystem.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include "thread.h"
#include "process.h"
#include "memorymanagement.h"
#include "initrd.h"
#include "elf.h"
#include "syscall.h"

using namespace Maxsi;

namespace Sortix
{
	bool ProcessSegment::Intersects(ProcessSegment* segments)
	{
		for ( ProcessSegment* tmp = segments; tmp != NULL; tmp = tmp->next )
		{
			if ( tmp->position < position + size &&
			     position < tmp->position + tmp->size )
			{
				return true;
			}
		}

		if ( next ) { return next->Intersects(segments); }

		return false;
	}

	ProcessSegment* ProcessSegment::Fork()
	{
		ProcessSegment* nextclone = NULL;
		if ( next )
		{
			nextclone = next->Fork();
			if ( nextclone == NULL ) { return NULL; }
		}

		ProcessSegment* clone = new ProcessSegment();
		if ( clone == NULL )
		{
			while ( nextclone != NULL )
			{
				ProcessSegment* todelete = nextclone;
				nextclone = nextclone->next;
				delete todelete;
			}

			return NULL;
		}

		next->prev = nextclone;
		clone->next = nextclone;
		clone->position = position;
		clone->size = size;

		return clone;
	}

	Process::Process()
	{
		addrspace = 0;
		segments = NULL;
		sigint = false;
		parent = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		firstchild = NULL;
		firstthread = NULL;
		mmapfrom = 0x80000000UL;
		pid = AllocatePID();
	}

	Process::~Process()
	{
		ResetAddressSpace();

		// Avoid memory leaks.
		ASSERT(segments == NULL);

		// TODO: Delete address space!
	}

	void Process::ResetAddressSpace()
	{
		ProcessSegment* tmp = segments;
		while ( tmp != NULL )
		{
			Memory::UnmapRangeUser(tmp->position, tmp->size);
			ProcessSegment* todelete = tmp;
			tmp = tmp->next;
			delete todelete;
		}

		segments = NULL;
	}

	Process* Process::Fork()
	{
		ASSERT(CurrentProcess() == this);

		Process* clone = new Process;
		if ( !clone ) { return NULL; }

		ProcessSegment* clonesegments = NULL;

		// Fork the segment list.
		if ( segments )
		{
			clonesegments = segments->Fork();
			if ( clonesegments == NULL ) { delete clone; return NULL; }
		}

		// Fork address-space here and copy memory somehow.
		clone->addrspace = Memory::Fork();
		if ( !clone->addrspace )
		{
			// Delete the segment list, since they are currently bogus.
			ProcessSegment* tmp = clonesegments;
			while ( tmp != NULL )
			{
				ProcessSegment* todelete = tmp;
				tmp = tmp->next;
				delete todelete;
			}

			delete clone; return NULL;
		}

		// Now it's too late to clean up here, if anything goes wrong, the
		// cloned process should be queued for destruction.
		clone->segments = clonesegments;

		// Remember the relation to the child process.
		clone->parent = this;
		if ( firstchild )
		{
			firstchild->prevsibling = clone;
			clone->nextsibling = firstchild;
			firstchild = clone;
		}
		else
		{
			firstchild = clone;
		}

		// Fork the file descriptors.
		if ( !descriptors.Fork(&clone->descriptors) )
		{
			Panic("No error handling when forking FDs fails!");
		}

		Thread* clonethreads = ForkThreads(clone);
		if ( !clonethreads )
		{
			Panic("No error handling when forking threads fails!");
		}

		clone->firstthread = clonethreads;

		// Copy variables.
		clone->mmapfrom = mmapfrom;

		// Now that the cloned process is fully created, we need to signal to
		// its threads that they should insert themselves into the scheduler.
		for ( Thread* tmp = clonethreads; tmp != NULL; tmp = tmp->nextsibling )
		{
			tmp->Ready();
		}

		return clone;
	}

	Thread* Process::ForkThreads(Process* processclone)
	{
		Thread* result = NULL;
		Thread* tmpclone = NULL;
		
		for ( Thread* tmp = firstthread; tmp != NULL; tmp = tmp->nextsibling )
		{
			Thread* clonethread = tmp->Fork();
			if ( clonethread == NULL )
			{
				while ( tmpclone != NULL )
				{
					Thread* todelete = tmpclone;
					tmpclone = tmpclone->prevsibling;
					delete todelete;
				}

				return NULL;
			}

			clonethread->process = processclone;

			if ( result == NULL ) { result = clonethread; }
			if ( tmpclone != NULL )
			{
				tmpclone->nextsibling = clonethread;
				clonethread->prevsibling = tmpclone;
			}

			tmpclone = clonethread;
		}

		return result;
	}

	void Process::ResetForExecute()
	{
		// TODO: Delete all threads and their stacks.
		// TODO: Unmap any process memory segments.
	}

	int Process::Execute(const char* programname, CPU::InterruptRegisters* regs)
	{
		size_t programsize = 0;
		byte* program = InitRD::Open(programname, &programsize);
		if ( !program ) { return -1; }

		addr_t entry = ELF::Construct(CurrentProcess(), program, programsize);
		if ( !entry )
		{
			Log::PrintF("Could not create process '%s'", programname);
			if ( String::Compare(programname, "sh") == 0 )
			{
				Panic("Couldn't create the shell process");
			}

			return Execute("sh", regs);
		}

		// TODO: This may be an ugly hack!
		// TODO: Move this to x86/process.cpp.
		regs->eip = entry;
		regs->useresp = CurrentThread()->stackpos + CurrentThread()->stacksize;
		regs->ebp = CurrentThread()->stackpos + CurrentThread()->stacksize;

		return 0;
	}

	int SysExecute(const char* programname)
	{
		// TODO: Validate that filepath is a user-space readable string! 

		// This is a hacky way to set up the thread!
		return Process::Execute(programname, Syscall::InterruptRegs());
	}

	pid_t SysFork()
	{
		// Prepare the state of the clone.
		Syscall::SyscallRegs()->result = 0;
		CurrentThread()->SaveRegisters(Syscall::InterruptRegs());

		Process* clone = CurrentProcess()->Fork();
		if ( !clone ) { return -1; }

		return clone->pid;
	}

	pid_t SysGetPID()
	{
		return CurrentProcess()->pid;
	}

	pid_t SysGetParentPID()
	{
		Process* parent = CurrentProcess()->parent;
		if ( !parent ) { return -1; }

		return parent->pid;
	}

	pid_t nextpidtoallocate;

	pid_t Process::AllocatePID()
	{
		return nextpidtoallocate++;
	}

	void Process::Init()
	{
		Syscall::Register(SYSCALL_EXEC, (void*) SysExecute);
		Syscall::Register(SYSCALL_FORK, (void*) SysFork);
		Syscall::Register(SYSCALL_GETPID, (void*) SysGetPID);
		Syscall::Register(SYSCALL_GETPPID, (void*) SysGetParentPID);

		nextpidtoallocate = 0;
	}

	addr_t Process::AllocVirtualAddr(size_t size)
	{
		return (mmapfrom -= size);
	}
}
