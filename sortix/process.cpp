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

	process.cpp
	A named collection of threads.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/worker.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/signal.h>
#include <sortix/unistd.h>
#include <sortix/fork.h>
#include <sortix/mman.h>
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

		if ( nextclone )
			nextclone->prev = clone;
		clone->next = nextclone;
		clone->position = position;
		clone->size = size;

		return clone;
	}

	Process::Process()
	{
		addrspace = 0;
		segments = NULL;
		parent = NULL;
		prevsibling = NULL;
		nextsibling = NULL;
		firstchild = NULL;
		zombiechild = NULL;
		parentlock = KTHREAD_MUTEX_INITIALIZER;
		childlock = KTHREAD_MUTEX_INITIALIZER;
		zombiecond = KTHREAD_COND_INITIALIZER;
		zombiewaiting = 0;
		iszombie = false;
		nozombify = false;
		firstthread = NULL;
		threadlock = KTHREAD_MUTEX_INITIALIZER;
		workingdir = NULL;
		mmapfrom = 0x80000000UL;
		exitstatus = -1;
		pid = AllocatePID();
		Put(this);
	}

	Process::~Process()
	{
		ASSERT(!zombiechild);
		ASSERT(!firstchild);
		ASSERT(!addrspace);
		ASSERT(!segments);

		Remove(this);
		delete[] workingdir;
	}

	void Process__OnLastThreadExit(void* user);

	void Process::OnThreadDestruction(Thread* thread)
	{
		ASSERT(thread->process == this);
		kthread_mutex_lock(&threadlock);
		if ( thread->prevsibling )
			thread->prevsibling->nextsibling = thread->nextsibling;
		if ( thread->nextsibling )
			thread->nextsibling->prevsibling = thread->prevsibling;
		if ( thread == firstthread )
			firstthread = thread->nextsibling;
		if ( firstthread )
			firstthread->prevsibling = NULL;
		thread->prevsibling = thread->nextsibling = NULL;
		bool threadsleft = firstthread;
		kthread_mutex_unlock(&threadlock);

		// We are called from the threads destructor, let it finish before we
		// we handle the situation by killing ourselves.
		if ( !threadsleft )
			ScheduleDeath();
	}

	void Process::ScheduleDeath()
	{
		// All our threads must have exited at this point.
		ASSERT(!firstthread);
		Worker::Schedule(Process__OnLastThreadExit, this);
	}

	// Useful for killing a partially constructed process without waiting for
	// it to die and garbage collect its zombie. It is not safe to access this
	// process after this call as another thread may garbage collect it.
	void Process::AbortConstruction()
	{
		nozombify = true;
		ScheduleDeath();
	}

	void Process__OnLastThreadExit(void* user)
	{
		return ((Process*) user)->OnLastThreadExit();
	}

	void Process::OnLastThreadExit()
	{
		LastPrayer();
	}

	static void SwitchCurrentAddrspace(addr_t addrspace, void* user)
	{
		((Thread*) user)->SwitchAddressSpace(addrspace);
	}

	void Process::LastPrayer()
	{
		ASSERT(this);
		// This must never be called twice.
		ASSERT(!iszombie);

		// This must be called from a thread using another address space as the
		// address space of this process is about to be destroyed.
		Thread* curthread = CurrentThread();
		ASSERT(curthread->process != this);

		// This can't be called if the process is still alive.
		ASSERT(!firstthread);

		// We need to temporarily reload the correct addrese space of the dying
		// process such that we can unmap and free its memory.
		addr_t prevaddrspace = curthread->SwitchAddressSpace(addrspace);

		ResetAddressSpace();
		descriptors.Reset();

		// Destroy the address space and safely switch to the replacement
		// address space before things get dangerous.
		Memory::DestroyAddressSpace(prevaddrspace,
                                    SwitchCurrentAddrspace,
                                    curthread);
		addrspace = 0;

		// Init is nice and will gladly raise our orphaned children and zombies.
		Process* init = Scheduler::GetInitProcess();
		ASSERT(init);
		kthread_mutex_lock(&childlock);
		while ( firstchild )
		{
			ScopedLock firstchildlock(&firstchild->parentlock);
			ScopedLock initlock(&init->childlock);
			Process* process = firstchild;
			firstchild = process->nextsibling;
			process->parent = init;
			process->prevsibling = NULL;
			process->nextsibling = init->firstchild;
			if ( init->firstchild )
				init->firstchild->prevsibling = process;
			init->firstchild = process;
		}
		// Since we have no more children (they are with init now), we don't
		// have to worry about new zombie processes showing up, so just collect
		// those that are left. Then we satisfiy the invariant !zombiechild that
		// applies on process termination.
		bool hadzombies = zombiechild;
		while ( zombiechild )
		{
			ScopedLock zombiechildlock(&zombiechild->parentlock);
			ScopedLock initlock(&init->childlock);
			Process* zombie = zombiechild;
			zombiechild = zombie->nextsibling;
			zombie->prevsibling = NULL;
			zombie->nextsibling = init->zombiechild;
			if ( init->zombiechild )
				init->zombiechild->prevsibling = zombie;
			init->zombiechild = zombie;
		}
		kthread_mutex_unlock(&childlock);

		if ( hadzombies )
			init->NotifyNewZombies();

		iszombie = true;

		bool zombify = !nozombify;

		// This class instance will be destroyed by our parent process when it
		// has received and acknowledged our death.
		kthread_mutex_lock(&parentlock);
		if ( parent )
			parent->NotifyChildExit(this, zombify);
		kthread_mutex_unlock(&parentlock);

		// If nobody is waiting for us, then simply commit suicide.
		if ( !zombify )
			delete this;
	}

	void Process::ResetAddressSpace()
	{
		ASSERT(Memory::GetAddressSpace() == addrspace);
		ProcessSegment* tmp = segments;
		while ( tmp != NULL )
		{
			Memory::UnmapRange(tmp->position, tmp->size);
			ProcessSegment* todelete = tmp;
			tmp = tmp->next;
			delete todelete;
		}

		segments = NULL;
	}

	void Process::NotifyChildExit(Process* child, bool zombify)
	{
		kthread_mutex_lock(&childlock);

		if ( child->prevsibling )
			child->prevsibling->nextsibling = child->nextsibling;
		if ( child->nextsibling )
			child->nextsibling->prevsibling = child->prevsibling;
		if ( firstchild == child )
			firstchild = child->nextsibling;
		if ( firstchild )
			firstchild->prevsibling = NULL;

		if ( zombify )
		{
			if ( zombiechild )
				zombiechild->prevsibling = child;
			child->prevsibling = NULL;
			child->nextsibling = zombiechild;
			zombiechild = child;
		}

		kthread_mutex_unlock(&childlock);

		if ( zombify )
			NotifyNewZombies();
	}

	void Process::NotifyNewZombies()
	{
		ScopedLock lock(&childlock);
		// TODO: Send SIGCHLD here?
		if ( zombiewaiting )
			kthread_cond_broadcast(&zombiecond);
	}

	pid_t Process::Wait(pid_t thepid, int* status, int options)
	{
		// TODO: Process groups are not supported yet.
		if ( thepid < -1 || thepid == 0 ) { Error::Set(ENOSYS); return -1; }

		ScopedLock lock(&childlock);

		// A process can only wait if it has children.
		if ( !firstchild && !zombiechild ) { Error::Set(ECHILD); return -1; }

		// Processes can only wait for their own children to exit.
		if ( 0 < thepid )
		{
			// TODO: This is a slow but multithread safe way to verify that the
			// target process has the correct parent.
			bool found = false;
			for ( Process* p = firstchild; !found && p; p = p->nextsibling )
				if ( p->pid == thepid )
					found = true;
			for ( Process* p = zombiechild; !found && p; p = p->nextsibling )
				if ( p->pid == thepid )
					found = true;
			if ( !found ) { Error::Set(ECHILD); return -1; }
		}

		Process* zombie = NULL;
		while ( !zombie )
		{
			for ( zombie = zombiechild; zombie; zombie = zombie->nextsibling )
				if ( thepid == -1 || thepid == zombie->pid )
					break;
			if ( zombie )
				break;
			zombiewaiting++;
			kthread_cond_wait(&zombiecond, &childlock);
			zombiewaiting--;
		}

		if ( zombie->prevsibling )
			zombie->prevsibling->nextsibling = zombie->nextsibling;
		if ( zombie->nextsibling )
			zombie->nextsibling->prevsibling = zombie->prevsibling;
		if ( zombiechild == zombie )
			zombiechild = zombie->nextsibling;
		if ( zombiechild )
			zombiechild->prevsibling = NULL;

		thepid = zombie->pid;

		int exitstatus = zombie->exitstatus;
		if ( exitstatus < 0 )
			exitstatus = 0;

		// TODO: Validate that status is a valid user-space int!
		if ( status )
			*status = exitstatus;

		// And so, the process was fully deleted.
		delete zombie;

		return thepid;
	}

	pid_t SysWait(pid_t pid, int* status, int options)
	{
		return CurrentProcess()->Wait(pid, status, options);
	}

	void Process::Exit(int status)
	{
		ScopedLock lock(&threadlock);
		// Status codes can only contain 8 bits according to ISO C and POSIX.
		if ( exitstatus == -1 )
			exitstatus = status % 256;

		// Broadcast SIGKILL to all our threads which will begin our long path
		// of process termination. We simply can't stop the threads as they may
		// be running in kernel mode doing dangerous stuff. This thread will be
		// destroyed by SIGKILL once the system call returns.
		for ( Thread* t = firstthread; t; t = t->nextsibling )
			t->DeliverSignal(SIGKILL);
	}

	void SysExit(int status)
	{
		CurrentProcess()->Exit(status);
	}

	bool Process::DeliverSignal(int signum)
	{
		// TODO: How to handle signals that kill the process?
		if ( firstthread )
			return firstthread->DeliverSignal(signum);
		Error::Set(EINIT);
		return false;
	}

	void Process::AddChildProcess(Process* child)
	{
		ScopedLock mylock(&childlock);
		ScopedLock itslock(&child->parentlock);
		ASSERT(!child->parent);
		ASSERT(!child->nextsibling);
		ASSERT(!child->prevsibling);
		child->parent = this;
		child->nextsibling = firstchild;
		child->prevsibling = NULL;
		if ( firstchild )
			firstchild->prevsibling = child;
		firstchild = child;
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

		// Fork address-space here and copy memory.
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

		// Now it's too late to clean up here, if anything goes wrong, we simply
		// ask the process to commit suicide before it goes live.
		clone->segments = clonesegments;

		// Remember the relation to the child process.
		AddChildProcess(clone);

		bool failure = false;

		if ( !descriptors.Fork(&clone->descriptors) )
			failure = true;

		clone->mmapfrom = mmapfrom;

		clone->workingdir = NULL;
		if ( workingdir && !(clone->workingdir = String::Clone(workingdir)) )
			failure = true;

		// If the proces creation failed, ask the process to commit suicide and
		// not become a zombie, as we don't wait for it to exit. It will clean
		// up all the above resources and delete itself.
		if ( failure )
		{
			clone->AbortConstruction();
			return NULL;
		}

		return clone;
	}

	void Process::ResetForExecute()
	{
		// TODO: Delete all threads and their stacks.

		ResetAddressSpace();
	}

	int Process::Execute(const char* programname, const byte* program,
	                     size_t programsize, int argc, const char* const* argv,
	                     int envc, const char* const* envp,
	                     CPU::InterruptRegisters* regs)
	{
		ASSERT(CurrentProcess() == this);

		addr_t entry = ELF::Construct(CurrentProcess(), program, programsize);
		if ( !entry ) { return -1; }

		// TODO: This may be an ugly hack!
		// TODO: Move this to x86/process.cpp.

		addr_t stackpos = CurrentThread()->stackpos + CurrentThread()->stacksize;

		// Alright, move argv onto the new stack! First figure out exactly how
		// big argv actually is.
		addr_t argvpos = stackpos - sizeof(char*) * (argc+1);
		char** stackargv = (char**) argvpos;

		size_t argvsize = 0;
		for ( int i = 0; i < argc; i++ )
		{
			size_t len = String::Length(argv[i]) + 1;
			argvsize += len;
			char* dest = ((char*) argvpos) - argvsize;
			stackargv[i] = dest;
			Maxsi::Memory::Copy(dest, argv[i], len);
		}

		stackargv[argc] = NULL;

		if ( argvsize % 16UL ) { argvsize += 16 - (argvsize % 16UL); }

		// And then move envp onto the stack.
		addr_t envppos = argvpos - argvsize - sizeof(char*) * (envc+1);
		char** stackenvp = (char**) envppos;

		size_t envpsize = 0;
		for ( int i = 0; i < envc; i++ )
		{
			size_t len = String::Length(envp[i]) + 1;
			envpsize += len;
			char* dest = ((char*) envppos) - envpsize;
			stackenvp[i] = dest;
			Maxsi::Memory::Copy(dest, envp[i], len);
		}

		stackenvp[envc] = NULL;

		if ( envpsize % 16UL ) { envpsize += 16 - (envpsize % 16UL); }

		stackpos = envppos - envpsize;

		descriptors.OnExecute();

		ExecuteCPU(argc, stackargv, envc, stackenvp, stackpos, entry, regs);

		return 0;
	}

	DevBuffer* OpenProgramImage(const char* progname, const char* wd, const char* path)
	{
		char* abs = Directory::MakeAbsolute("/", progname);
		if ( !abs ) { Error::Set(ENOMEM); return NULL; }

		// TODO: Use O_EXEC here!
		Device* dev =  FileSystem::Open(abs, O_RDONLY, 0);
		delete[] abs;

		if ( !dev ) { return NULL; }
		if ( !dev->IsType(Device::BUFFER) ) { Error::Set(EACCES); dev->Unref(); return NULL; }
		return (DevBuffer*) dev;
	}

	int SysExecVE(const char* _filename, char* const _argv[], char* const _envp[])
	{
		char* filename;
		int argc;
		int envc;
		char** argv;
		char** envp;
		DevBuffer* dev;
		uintmax_t needed;
		size_t sofar;
		size_t count;
		uint8_t* buffer;
		int result = -1;
		Process* process = CurrentProcess();
		CPU::InterruptRegisters regs;
		Maxsi::Memory::Set(&regs, 0, sizeof(regs));

		filename = String::Clone(_filename);
		if ( !filename ) { goto cleanup_done; }

		for ( argc = 0; _argv && _argv[argc]; argc++ );
		for ( envc = 0; _envp && _envp[envc]; envc++ );

		argv = new char*[argc+1];
		if ( !argv ) { goto cleanup_filename; }
		Maxsi::Memory::Set(argv, 0, sizeof(char*) * (argc+1));

		for ( int i = 0; i < argc; i++ )
		{
			argv[i] = String::Clone(_argv[i]);
			if ( !argv[i] ) { goto cleanup_argv; }
		}

		envp = new char*[envc+1];
		if ( !envp ) { goto cleanup_argv; }
		envc = envc;
		Maxsi::Memory::Set(envp, 0, sizeof(char*) * (envc+1));

		for ( int i = 0; i < envc; i++ )
		{
			envp[i] = String::Clone(_envp[i]);
			if ( !envp[i] ) { goto cleanup_envp; }
		}

		dev = OpenProgramImage(filename, process->workingdir, "/bin");
		if ( !dev ) { goto cleanup_envp; }

		dev->Refer(); // TODO: Rules of GC may change soon.
		needed = dev->Size();
		if ( SIZE_MAX < needed ) { Error::Set(ENOMEM); goto cleanup_dev; }

		if ( !dev->IsReadable() ) { Error::Set(EBADF); goto cleanup_dev; }

		count = needed;
		buffer = new byte[count];
		if ( !buffer ) { goto cleanup_dev; }
		sofar = 0;
		while ( sofar < count )
		{
			ssize_t bytesread = dev->Read(buffer + sofar, count - sofar);
			if ( bytesread < 0 ) { goto cleanup_buffer; }
			if ( bytesread == 0 ) { Error::Set(EEOF); return -1; }
			sofar += bytesread;
		}

		result = process->Execute(filename, buffer, count, argc, argv, envc,
		                          envp, &regs);

	cleanup_buffer:
		delete[] buffer;
	cleanup_dev:
		dev->Unref();
	cleanup_envp:
		for ( int i = 0; i < envc; i++) { delete[] envp[i]; }
		delete[] envp;
	cleanup_argv:
		for ( int i = 0; i < argc; i++) { delete[] argv[i]; }
		delete[] argv;
	cleanup_filename:
		delete[] filename;
	cleanup_done:
		if ( !result ) { CPU::LoadRegisters(&regs); }
		return result;
	}

	pid_t SysTFork(int flags, tforkregs_t* regs)
	{
		if ( Signal::IsPending() ) { Error::Set(EINTR); return -1; }

		// TODO: Properly support tfork(2).
		if ( flags != SFFORK ) { Error::Set(ENOSYS); return -1; }

		CPU::InterruptRegisters cpuregs;
		InitializeThreadRegisters(&cpuregs, regs);

		// TODO: Is it a hack to create a new kernel stack here?
		Thread* curthread = CurrentThread();
		uint8_t* newkernelstack = new uint8_t[curthread->kernelstacksize];
		if ( !newkernelstack ) { return -1; }

		Process* clone = CurrentProcess()->Fork();
		if ( !clone ) { delete[] newkernelstack; return -1; }

		// If the thread could not be created, make the process commit suicide
		// in a manner such that we don't wait for its zombie.
		Thread* thread = CreateKernelThread(clone, &cpuregs);
		if ( !thread )
		{
			clone->AbortConstruction();
			return -1;
		}

		thread->kernelstackpos = (addr_t) newkernelstack;
		thread->kernelstacksize = curthread->kernelstacksize;
		thread->kernelstackmalloced = true;
		thread->stackpos = curthread->stackpos;
		thread->stacksize = curthread->stacksize;
		thread->sighandler = curthread->sighandler;

		StartKernelThread(thread);

		return clone->pid;
	}

	pid_t SysGetPID()
	{
		return CurrentProcess()->pid;
	}

	pid_t Process::GetParentProcessId()
	{
		ScopedLock lock(&parentlock);
		if( !parent )
			return 0;
		return parent->pid;
	}

	pid_t SysGetParentPID()
	{
		return CurrentProcess()->GetParentProcessId();
	}

	pid_t nextpidtoallocate;
	kthread_mutex_t pidalloclock;

	pid_t Process::AllocatePID()
	{
		ScopedLock lock(&pidalloclock);
		return nextpidtoallocate++;
	}

	// TODO: This is not thread safe.
	pid_t Process::HackGetForegroundProcess()
	{
		for ( pid_t i = nextpidtoallocate; 1 <= i; i-- )
		{
			Process* process = Get(i);
			if ( !process )
				continue;
			if ( process->pid <= 1 )
				continue;
			if ( process->iszombie )
				continue;
			return i;
		}
		return 0;
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
		ScopedLock lock(&pidalloclock);
		size_t index = pidlist->Search(ProcessPIDCompare, pid);
		if ( index == SIZE_MAX ) { return NULL; }

		return pidlist->Get(index);
	}

	bool Process::Put(Process* process)
	{
		ScopedLock lock(&pidalloclock);
		return pidlist->Add(process);
	}

	void Process::Remove(Process* process)
	{
		ScopedLock lock(&pidalloclock);
		size_t index = pidlist->Search(process);
		ASSERT(index != SIZE_MAX);

		pidlist->Remove(index);
	}

	void* SysSbrk(intptr_t increment)
	{
		Process* process = CurrentProcess();
		ProcessSegment* dataseg = NULL;
		for ( ProcessSegment* iter = process->segments; iter; iter = iter->next )
		{
			if ( !iter->type == SEG_DATA ) { continue; }
			if ( dataseg && iter->position < dataseg->position ) { continue; }
			dataseg = iter;
		}
		if ( !dataseg ) { Error::Set(ENOMEM); return (void*) -1UL; }
		addr_t currentend = dataseg->position + dataseg->size;
		addr_t newend = currentend + increment;
		if ( newend < dataseg->position ) { Error::Set(EINVAL); return (void*) -1UL; }
		if ( newend < currentend )
		{
			addr_t unmapfrom = Page::AlignUp(newend);
			if ( unmapfrom < currentend )
			{
				size_t unmapbytes = Page::AlignUp(currentend - unmapfrom);
				Memory::UnmapRange(unmapfrom, unmapbytes);
			}
		}
		else if ( currentend < newend )
		{
			// TODO: HACK: Make a safer way of expanding the data segment
			// without segments possibly colliding!
			addr_t mapfrom = Page::AlignUp(currentend);
			if ( mapfrom < newend )
			{
				size_t mapbytes = Page::AlignUp(newend - mapfrom);
				int prot = PROT_FORK | PROT_READ | PROT_WRITE | PROT_KREAD | PROT_KWRITE;
				if ( !Memory::MapRange(mapfrom, mapbytes, prot) )
				{
					return (void*) -1UL;
				}
			}
		}
		dataseg->size += increment;
		return (void*) newend;
	}

	size_t SysGetPageSize()
	{
		return Page::Size();
	}

	void Process::Init()
	{
		Syscall::Register(SYSCALL_EXEC, (void*) SysExecVE);
		Syscall::Register(SYSCALL_TFORK, (void*) SysTFork);
		Syscall::Register(SYSCALL_GETPID, (void*) SysGetPID);
		Syscall::Register(SYSCALL_GETPPID, (void*) SysGetParentPID);
		Syscall::Register(SYSCALL_EXIT, (void*) SysExit);
		Syscall::Register(SYSCALL_WAIT, (void*) SysWait);
		Syscall::Register(SYSCALL_SBRK, (void*) SysSbrk);
		Syscall::Register(SYSCALL_GET_PAGE_SIZE, (void*) SysGetPageSize);

		pidalloclock = KTHREAD_MUTEX_INITIALIZER;
		nextpidtoallocate = 0;

		pidlist = new SortedList<Process*>(ProcessCompare);
		if ( !pidlist ) { Panic("could not allocate pidlist\n"); }
	}

	addr_t Process::AllocVirtualAddr(size_t size)
	{
		return (mmapfrom -= size);
	}
}
