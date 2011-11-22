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
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include <libmaxsi/sortedlist.h>
#include "thread.h"
#include "process.h"
#include "device.h"
#include "stream.h"
#include "filesystem.h"
#include "directory.h"
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
		workingdir = NULL;
		errno = NULL;
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
	
		delete[] workingdir;

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
		errno = NULL;
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
		clone->errno = errno;
		if ( workingdir ) { clone->workingdir = String::Clone(workingdir); }
		else { clone->workingdir = NULL; }

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
	}

	int Process::Execute(const char* programname, const byte* program, size_t programsize, int argc, const char* const* argv, CPU::InterruptRegisters* regs)
	{
		ASSERT(CurrentProcess() == this);

		addr_t entry = ELF::Construct(CurrentProcess(), program, programsize);
		if ( !entry ) { return -1; }

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

	class SysExecVEState
	{
	public:
		char* filename;
		DevBuffer* dev;
		byte* buffer;
		size_t count;
		size_t sofar;
		int argc;
		char** argv;

	public:
		SysExecVEState()
		{
			filename = NULL;
			dev = NULL;
			buffer = NULL;
			count = 0;
			sofar = 0;
			argc = 0;
			argv = NULL;
		}

		~SysExecVEState()
		{
			delete[] filename;
			dev->Unref();
			delete[] buffer;
			for ( int i = 0; i < argc; i++ ) { delete[] argv[i]; }
			delete[] argv;
		}
	
	};

	int SysExevVEStage2(SysExecVEState* state)
	{
		if ( !state->dev->IsReadable() ) { Error::Set(EBADF); delete state; return -1; }

		byte* dest = state->buffer + state->sofar;
		size_t amount = state->count - state->sofar;
		ssize_t bytesread = state->dev->Read(dest, amount);

		// Check for premature end-of-file.
		if ( bytesread == 0 && amount != 0 )
		{
			Error::Set(EIO); delete state; return -1;
		}

		// We actually managed to read some data.
		if ( 0 <= bytesread )
		{
			state->sofar += bytesread;
			if ( state->sofar <= state->count )
			{
				CPU::InterruptRegisters* regs = Syscall::InterruptRegs();
				Process* process = CurrentProcess();
				int result = process->Execute(state->filename, state->buffer, state->count, state->argc, state->argv, regs);
				if ( result == 0 ) { Syscall::AsIs(); }
				delete state;
				return result;
			}

			return SysExevVEStage2(state);
		}

		if ( Error::Last() != EWOULDBLOCK ) { delete state; return -1; }

		// The stream will resume our system call once progress has been
		// made. Our request is certainly not forgotten.

		// Resume the system call with these parameters.
		Thread* thread = CurrentThread();
		thread->scfunc = (void*) SysExevVEStage2;
		thread->scstate[0] = (size_t) state;
		thread->scsize = sizeof(state);

		// Now go do something else.
		Syscall::Incomplete();
		return 0;
	}

	DevBuffer* OpenProgramImage(const char* progname, const char* wd, const char* path)
	{
		// TODO: Use the PATH enviromental variable.
		const char* base = ( *progname == '.' ) ? wd : path;
		char* abs = Directory::MakeAbsolute(base, progname);
		if ( !abs ) { Error::Set(ENOMEM); return NULL; }

		// TODO: Use O_EXEC here!
		Device* dev =  FileSystem::Open(abs, O_RDONLY, 0);
		delete[] abs;

		if ( !dev ) { return NULL; }
		if ( !dev->IsType(Device::BUFFER) ) { Error::Set(EACCESS); dev->Unref(); return NULL; }
		return (DevBuffer*) dev;
	}

	int SysExecVE(const char* filename, int argc, char* const argv[], char* const /*envp*/[])
	{
		// TODO: Validate that all the pointer-y parameters are SAFE!

		// Use a container class to store everything and handle cleaning up.
		SysExecVEState* state = new SysExecVEState;
		if ( !state ) { return -1; }

		// Make a copy of argv and filename as they are going to be destroyed
		// when the address space is reset.
		state->filename = String::Clone(filename);
		if ( !state->filename ) { delete state; return -1; }

		state->argc = argc;
		state->argv = new char*[state->argc];
		Maxsi::Memory::Set(state->argv, 0, sizeof(char*) * state->argc);
		if ( !state->argv ) { delete state; return -1; }

		for ( int i = 0; i < state->argc; i++ )
		{
			state->argv[i] = String::Clone(argv[i]);
			if ( !state->argv[i] ) { delete state; return -1; }
		}

		Process* process = CurrentProcess();
		state->dev = OpenProgramImage(state->filename, process->workingdir, "/bin");
		if ( !state->dev ) { delete state; return -1; }

		state->dev->Refer(); // TODO: Rules of GC may change soon.
		uintmax_t needed = state->dev->Size();
		if ( SIZE_MAX < needed ) { Error::Set(ENOMEM); delete state; return -1; }

		state->count = needed;
		state->buffer = new byte[state->count];
		if ( !state->buffer ) { delete state; return -1; }

		return SysExevVEStage2(state);
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

	void Process::Exit(int status)
	{
	// Status codes can only contain 8 bits according to ISO C and POSIX.
		status %= 256;

		ASSERT(this == CurrentProcess());

		Process* init = Scheduler::GetInitProcess();

		if ( pid == 0 ) { Panic("System idle process exited"); }

		// If the init process terminated successfully, time to halt.
		if ( this == init )
		{
			switch ( status )
			{
				case 0: CPU::ShutDown();
				case 1: CPU::Reboot();
				default: PanicF("The init process exited abnormally with status code %u\n", status);
			}
		}

		// Take care of the orphans, so give them to init.
		while ( firstchild )
		{
			Process* orphan = firstchild;
			firstchild = orphan->nextsibling;
			if ( firstchild ) { firstchild->prevsibling = NULL; }
			orphan->parent = init;
			orphan->prevsibling = NULL;
			orphan->nextsibling = init->firstchild;
			if ( orphan->nextsibling ) { orphan->nextsibling->prevsibling = orphan; }
			init->firstchild = orphan;
		}

		// Remove the current process from the family tree.
		if ( !prevsibling )
		{
			parent->firstchild = nextsibling;
		}
		else
		{
			prevsibling->nextsibling = nextsibling;
		}

		if ( nextsibling )
		{
			nextsibling->prevsibling = prevsibling;
		}

		// Close all the file descriptors.
		descriptors.Reset();

		// Make all threads belonging to process unrunnable.
		for ( Thread* t = firstthread; t; t = t->nextsibling )
		{
			Scheduler::EarlyWakeUp(t);
			Scheduler::SetThreadState(t, Thread::State::NONE);
		}

		// Delete the threads.
		while ( firstthread )
		{
			Thread* todelete = firstthread;
			firstthread = firstthread->nextsibling;
			delete todelete;
		}

		// Now clean up the address space.
		ResetAddressSpace();

		// TODO: Actually delete the address space. This is a small memory leak
		// of a couple pages.

		exitstatus = status;
		nextsibling = parent->zombiechild;
		if ( parent->zombiechild ) { parent->zombiechild->prevsibling = this; }
		parent->zombiechild = this;

		// Notify the parent process that the child has become a zombie.
		parent->OnChildProcessExit(this);
	}

	void SysExit(int status)
	{
		CurrentProcess()->Exit(status);

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

	int SysRegisterErrno(int* errnop)
	{
		CurrentProcess()->errno = errnop;
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
		Syscall::Register(SYSCALL_REGISTER_ERRNO, (void*) SysRegisterErrno);

		nextpidtoallocate = 0;

		pidlist = new SortedList<Process*>(ProcessCompare);
		if ( !pidlist ) { Panic("could not allocate pidlist\n"); }
	}

	addr_t Process::AllocVirtualAddr(size_t size)
	{
		return (mmapfrom -= size);
	}
}
