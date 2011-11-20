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
#include <libmaxsi/sortedlist.h>
#include "thread.h"
#include "process.h"
#include "device.h"
#include "scheduler.h"
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
		zombiechild = NULL;
		firstthread = NULL;
		mmapfrom = 0x80000000UL;
		exitstatus = -1;
		pid = AllocatePID();
		Put(this);
	}

	Process::~Process()
	{
		Remove(this);

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

		ResetAddressSpace();

		// HACK: Don't let VGA buffers survive executes.
		// Real Solution: Don't use VGA buffers in this manner.
		for ( int i = 0; i < 32; i++ )
		{
			Device* dev = descriptors.Get(i);
			if ( !dev ) { continue; }
			if ( dev->IsType(Device::VGABUFFER) ) { descriptors.Free(i); }
		}
	}

	int Process::Execute(const char* programname, int argc, const char* const* argv, CPU::InterruptRegisters* regs)
	{
		ASSERT(CurrentProcess() == this);

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

			const char* const SHARGV[]= { "sh" };
			return Execute("sh", 1, SHARGV, regs);
		}

		// TODO: This may be an ugly hack!
		// TODO: Move this to x86/process.cpp.

		// Alright, move argv onto the new stack! First figure out exactly how
		// big argv actually is.
		addr_t stackpos = CurrentThread()->stackpos + CurrentThread()->stacksize;
		addr_t argvpos = stackpos - sizeof(char*) * argc;
		char** stackargv = (char**) argvpos;
		regs->eax = argc;
		regs->ebx = argvpos;

		size_t argvsize = 0;
		for ( int i = 0; i < argc; i++ )
		{
			size_t len = String::Length(argv[i]) + 1;
			argvsize += len;
			char* dest = ((char*) argvpos) - argvsize;
			stackargv[i] = dest;
			Maxsi::Memory::Copy(dest, argv[i], len);
		}

		stackpos = argvpos - argvsize;

		regs->eip = entry;
		regs->useresp = stackpos;
		regs->ebp = stackpos;

		return 0;
	}

	int SysExecVE(const char* filename, int argc, char* const argv[], char* const /*envp*/[])
	{
		// TODO: Validate that all the pointer-y parameters are SAFE!

		// Make a copy of argv and filename as they are going to be destroyed
		// when the address space is reset.
		filename = String::Clone(filename);
		if ( !filename ) { return -1; /* TODO: errno */ }

		char** newargv = new char*[argc];
		if ( !newargv ) { delete[] filename; return -1; /* TODO: errno */ }

		for ( int i = 0; i < argc; i++ )
		{
			newargv[i] = String::Clone(argv[i]);
			if ( !newargv[i] )
			{
				while ( i ) { delete[] newargv[--i]; }

				return -1; /* TODO: errno */
			}
		}

		argv = newargv;

		CPU::InterruptRegisters* regs = Syscall::InterruptRegs();
		Process* process = CurrentProcess();
		int result = process->Execute(filename, argc, argv, regs);
		Syscall::AsIs();

		for ( int i = 0; i < argc; i++ ) { delete[] argv[i]; }
		delete[] argv;
		delete[] filename;

		return result;
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

	int ProcessCompare(Process* a, Process* b)
	{
		if ( a->pid < b->pid ) { return -1; }
		if ( a->pid > b->pid ) { return 1; }
		return 0;
	}

	int ProcessPIDCompare(Process* a, pid_t pid)
	{
		if ( a->pid < pid ) { return -1; }
		if ( a->pid > pid ) { return 1; }
		return 0;
	}

	SortedList<Process*>* pidlist;

	Process* Process::Get(pid_t pid)
	{
		size_t index = pidlist->Search(ProcessPIDCompare, pid);
		if ( index == SIZE_MAX ) { return NULL; }

		return pidlist->Get(index);
	}

	bool Process::Put(Process* process)
	{
		return pidlist->Add(process);
	}

	void Process::Remove(Process* process)
	{
		size_t index = pidlist->Search(process);
		ASSERT(index != SIZE_MAX);

		pidlist->Remove(index);
	}

	void Process::OnChildProcessExit(Process* process)
	{
		ASSERT(process->parent == this);

		for ( Thread* thread = firstthread; thread; thread = thread->nextsibling )
		{
			if ( thread->onchildprocessexit )
			{
				thread->onchildprocessexit(thread, process);
			}
		}
	}

	void SysExit(int status)
	{
		// Status codes can only contain 8 bits according to ISO C and POSIX.
		status %= 256;

		Process* process = CurrentProcess();
		Process* parent = process->parent;
		Process* init = Scheduler::GetInitProcess();

		if ( process->pid == 0 ) { Panic("System idle process exited"); }

		// If the init process terminated successfully, time to halt.
		if ( process == init )
		{
			switch ( status )
			{
				case 0: CPU::ShutDown();
				case 1: CPU::Reboot();
				default: PanicF("The init process exited abnormally with status code %u\n", status);
			}
		}

		// Take care of the orphans, so give them to init.
		while ( process->firstchild )
		{
			Process* orphan = process->firstchild;
			process->firstchild = orphan->nextsibling;
			if ( process->firstchild ) { process->firstchild->prevsibling = NULL; }
			orphan->parent = init;
			orphan->prevsibling = NULL;
			orphan->nextsibling = init->firstchild;
			if ( orphan->nextsibling ) { orphan->nextsibling->prevsibling = orphan; }
			init->firstchild = orphan;
		}

		// Remove the current process from the family tree.
		if ( !process->prevsibling )
		{
			parent->firstchild = process->nextsibling;
		}
		else
		{
			process->prevsibling->nextsibling = process->nextsibling;
		}

		if ( process->nextsibling )
		{
			process->nextsibling->prevsibling = process->prevsibling;
		}

		// TODO: Close all the file descriptors!

		// Make all threads belonging to process unrunnable.
		for ( Thread* t = process->firstthread; t; t = t->nextsibling )
		{
			Scheduler::EarlyWakeUp(t);
			Scheduler::SetThreadState(t, Thread::State::NONE);
		}

		// Delete the threads.
		while ( process->firstthread )
		{
			Thread* todelete = process->firstthread;
			process->firstthread = process->firstthread->nextsibling;
			delete todelete;
		}

		// Now clean up the address space.
		process->ResetAddressSpace();

		// TODO: Actually delete the address space. This is a small memory leak
		// of a couple pages.

		process->exitstatus = status;
		process->nextsibling = parent->zombiechild;
		if ( parent->zombiechild ) { parent->zombiechild->prevsibling = process; }
		parent->zombiechild = process;

		// Notify the parent process that the child has become a zombie.
		parent->OnChildProcessExit(process);

		// And so, the process had vanished from existence. But as fate would
		// have it, soon a replacement took its place.
		Scheduler::ProcessTerminated(Syscall::InterruptRegs());
		Syscall::AsIs();
	}

	struct SysWait_t
	{
		union { size_t align1; pid_t pid; };
		union { size_t align2; int* status; };
		union { size_t align3; int options; };
	};

	STATIC_ASSERT(sizeof(SysWait_t) <= sizeof(Thread::scstate));

	void SysWaitCallback(Thread* thread, Process* exitee)
	{
		// See if this process matches what we are looking for.
		SysWait_t* state = (SysWait_t*) thread->scstate;
		if ( state->pid != -1 && state->pid != exitee->pid ) { return; }

		thread->onchildprocessexit = NULL;

		Syscall::ScheduleResumption(thread);
	}

	pid_t SysWait(pid_t pid, int* status, int options)
	{
		Thread* thread = CurrentThread();
		Process* process = thread->process;

		if ( pid != -1 )
		{
			Process* waitingfor = Process::Get(pid);
			if ( !waitingfor ) { return -1; /* TODO: ECHILD*/ }
			if ( waitingfor->parent != process ) { return -1; /* TODO: ECHILD*/ }
		}

		// Find any zombie children matching the search description.
		for ( Process* zombie = process->zombiechild; zombie; zombie = zombie->nextsibling )
		{
			if ( pid != -1 && pid != zombie->pid ) { continue; }

			pid = zombie->pid;
			// TODO: Validate that status is a valid user-space int!
			if ( status ) { *status = zombie->exitstatus; }

			if ( zombie == process->zombiechild )
			{
				process->zombiechild = zombie->nextsibling;
				if ( zombie->nextsibling ) { zombie->nextsibling->prevsibling = NULL; }
			}
			else
			{
				zombie->prevsibling->nextsibling = zombie->nextsibling;
				if ( zombie->nextsibling ) { zombie->nextsibling->prevsibling = zombie->prevsibling; }
			}

			// And so, the process was fully deleted.
			delete zombie;

			return pid;
		}

		// The process needs to have children, otherwise we are waiting for
		// nothing to happen.
		if ( !process->firstchild ) { return -1; /* TODO: ECHILD*/ }
		
		// Resumes this system call when the wait condition has been met.
		thread->onchildprocessexit = SysWaitCallback;

		// Resume the system call with these parameters.
		thread->scfunc = (void*) SysWait;
		SysWait_t* state = (SysWait_t*) thread->scstate;
		state->pid = pid;
		state->status = status;
		state->options = options;
		thread->scsize = sizeof(SysWait_t);

		// Now go do something else.
		Syscall::Incomplete();
		return 0;
	}

	void Process::Init()
	{
		Syscall::Register(SYSCALL_EXEC, (void*) SysExecVE);
		Syscall::Register(SYSCALL_FORK, (void*) SysFork);
		Syscall::Register(SYSCALL_GETPID, (void*) SysGetPID);
		Syscall::Register(SYSCALL_GETPPID, (void*) SysGetParentPID);
		Syscall::Register(SYSCALL_EXIT, (void*) SysExit);
		Syscall::Register(SYSCALL_WAIT, (void*) SysWait);

		nextpidtoallocate = 0;

		pidlist = new SortedList<Process*>(ProcessCompare);
		if ( !pidlist ) { Panic("could not allocate pidlist\n"); }
	}

	addr_t Process::AllocVirtualAddr(size_t size)
	{
		return (mmapfrom -= size);
	}
}
