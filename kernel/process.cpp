/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/clock.h>
#include <sortix/fcntl.h>
#include <sortix/fork.h>
#include <sortix/mman.h>
#include <sortix/resource.h>
#include <sortix/signal.h>
#include <sortix/stat.h>
#include <sortix/unistd.h>
#include <sortix/wait.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/sortedlist.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/symbol.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/worker.h>

#include "elf.h"
#include "initrd.h"

namespace Sortix {

Process::Process()
{
	string_table = NULL;
	string_table_length = 0;
	symbol_table = NULL;
	symbol_table_length = 0;
	addrspace = 0;
	segments = NULL;
	parent = NULL;
	prevsibling = NULL;
	nextsibling = NULL;
	firstchild = NULL;
	zombiechild = NULL;
	group = NULL;
	groupprev = NULL;
	groupnext = NULL;
	groupfirst = NULL;
	program_image_path = NULL;
	parentlock = KTHREAD_MUTEX_INITIALIZER;
	childlock = KTHREAD_MUTEX_INITIALIZER;
	groupparentlock = KTHREAD_MUTEX_INITIALIZER;
	groupchildlock = KTHREAD_MUTEX_INITIALIZER;
	groupchildleft = KTHREAD_COND_INITIALIZER;
	zombiecond = KTHREAD_COND_INITIALIZER;
	zombiewaiting = 0;
	iszombie = false;
	nozombify = false;
	grouplimbo = false;
	firstthread = NULL;
	threadlock = KTHREAD_MUTEX_INITIALIZER;
	ptrlock = KTHREAD_MUTEX_INITIALIZER;
	idlock = KTHREAD_MUTEX_INITIALIZER;
	user_timers_lock = KTHREAD_MUTEX_INITIALIZER;
	segments = NULL;
	segments_used = 0;
	segments_length = 0;
	segment_lock = KTHREAD_MUTEX_INITIALIZER;
	exitstatus = -1;
	pid = AllocatePID();
	uid = euid = 0;
	gid = egid = 0;
	umask = 0022;
	nicelock = KTHREAD_MUTEX_INITIALIZER;
	nice = 0;
	resource_limits_lock = KTHREAD_MUTEX_INITIALIZER;
	for ( size_t i = 0; i < RLIMIT_NUM_DECLARED; i++ )
	{
		resource_limits[i].rlim_cur = RLIM_INFINITY;
		resource_limits[i].rlim_max = RLIM_INFINITY;
	}
	Time::InitializeProcessClocks(this);
	alarm_timer.Attach(Time::GetClock(CLOCK_MONOTONIC));
	Put(this);
}

Process::~Process()
{
	delete[] string_table;
	delete[] symbol_table;
	if ( alarm_timer.IsAttached() )
		alarm_timer.Detach();
	delete[] program_image_path;
	assert(!zombiechild);
	assert(!firstchild);
	assert(!addrspace);
	assert(!segments);
	assert(!dtable);
	assert(!mtable);
	assert(!cwd);
	assert(!root);

	Remove(this);
}

void Process::BootstrapTables(Ref<DescriptorTable> dtable, Ref<MountTable> mtable)
{
	ScopedLock lock(&ptrlock);
	assert(!this->dtable);
	assert(!this->mtable);
	this->dtable = dtable;
	this->mtable = mtable;
}

void Process::BootstrapDirectories(Ref<Descriptor> root)
{
	ScopedLock lock(&ptrlock);
	assert(!this->root);
	assert(!this->cwd);
	this->root = root;
	this->cwd = root;
}

void Process__OnLastThreadExit(void* user);

void Process::OnThreadDestruction(Thread* thread)
{
	assert(thread->process == this);
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
	assert(!firstthread);
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

void Process::DeleteTimers()
{
	for ( timer_t i = 0; i < PROCESS_TIMER_NUM_MAX; i++ )
	{
		if ( user_timers[i].timer.IsAttached() )
		{
			user_timers[i].timer.Cancel();
			user_timers[i].timer.Detach();
		}
	}
}

void Process::LastPrayer()
{
	assert(this);
	// This must never be called twice.
	assert(!iszombie);

	// This must be called from a thread using another address space as the
	// address space of this process is about to be destroyed.
	Thread* curthread = CurrentThread();
	assert(curthread->process != this);

	// This can't be called if the process is still alive.
	assert(!firstthread);

	// Disarm and detach all the timers in the process.
	DeleteTimers();
	if ( alarm_timer.IsAttached() )
	{
		alarm_timer.Cancel();
		alarm_timer.Detach();
	}

	// We need to temporarily reload the correct addrese space of the dying
	// process such that we can unmap and free its memory.
	addr_t prevaddrspace = curthread->SwitchAddressSpace(addrspace);

	ResetAddressSpace();

	if ( dtable ) dtable.Reset();
	if ( cwd ) cwd.Reset();
	if ( root ) root.Reset();
	if ( mtable ) mtable.Reset();

	// Destroy the address space and safely switch to the replacement
	// address space before things get dangerous.
	Memory::DestroyAddressSpace(prevaddrspace,
                                SwitchCurrentAddrspace,
                                curthread);
	addrspace = 0;

	// Unload the process symbol and string tables.
	delete[] symbol_table; symbol_table = NULL;
	delete[] string_table; string_table = NULL;

	// Init is nice and will gladly raise our orphaned children and zombies.
	Process* init = Scheduler::GetInitProcess();
	assert(init);
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

	// Remove ourself from our process group.
	kthread_mutex_lock(&groupchildlock);
	if ( group )
		group->NotifyMemberExit(this);
	kthread_mutex_unlock(&groupchildlock);

	// This class instance will be destroyed by our parent process when it
	// has received and acknowledged our death.
	kthread_mutex_lock(&parentlock);
	if ( parent )
		parent->NotifyChildExit(this, zombify);
	kthread_mutex_unlock(&parentlock);

	// If nobody is waiting for us, then simply commit suicide.
	if ( !zombify )
	{
		kthread_mutex_lock(&groupparentlock);
		bool in_limbo = groupfirst && (grouplimbo = true);
		kthread_mutex_unlock(&groupparentlock);

		if ( !in_limbo )
			delete this;
	}
}

void Process::ResetAddressSpace()
{
	ScopedLock lock(&segment_lock);

	assert(Memory::GetAddressSpace() == addrspace);

	for ( size_t i = 0; i < segments_used; i++ )
		Memory::UnmapRange(segments[i].addr, segments[i].size);

	Memory::Flush();

	segments_used = segments_length = 0;
	free(segments);
	segments = NULL;
}

void Process::NotifyMemberExit(Process* child)
{
	assert(child->group == this);
	kthread_mutex_lock(&groupparentlock);
	if ( child->groupprev )
		child->groupprev->groupnext = child->groupnext;
	else
		groupfirst = child->groupnext;
	if ( child->groupnext )
		child->groupnext->groupprev = child->groupprev;
	kthread_cond_signal(&groupchildleft);
	kthread_mutex_unlock(&groupparentlock);

	child->group = NULL;

	NotifyLeftProcessGroup();
}

void Process::NotifyLeftProcessGroup()
{
	ScopedLock parentlock(&groupparentlock);
	if ( !grouplimbo || groupfirst )
		return;
	grouplimbo = false;
	delete this;
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

pid_t Process::Wait(pid_t thepid, int* user_status, int options)
{
	// TODO: Process groups are not supported yet.
	if ( thepid < -1 || thepid == 0 ) { errno = ENOSYS; return -1; }

	ScopedLock lock(&childlock);

	// A process can only wait if it has children.
	if ( !firstchild && !zombiechild )
		return errno = ECHILD, -1;

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
		if ( !found )
			return errno = ECHILD, -1;
	}

	Process* zombie = NULL;
	while ( !zombie )
	{
		for ( zombie = zombiechild; zombie; zombie = zombie->nextsibling )
			if ( thepid == -1 || thepid == zombie->pid )
				break;
		if ( zombie )
			break;
		if ( options & WNOHANG )
			return 0;
		zombiewaiting++;
		unsigned long r = kthread_cond_wait_signal(&zombiecond, &childlock);
		zombiewaiting--;
		if ( !r )
			return errno = EINTR, -1;
	}

	// Remove from the list of zombies.
	if ( zombie->prevsibling )
		zombie->prevsibling->nextsibling = zombie->nextsibling;
	if ( zombie->nextsibling )
		zombie->nextsibling->prevsibling = zombie->prevsibling;
	if ( zombiechild == zombie )
		zombiechild = zombie->nextsibling;
	if ( zombiechild )
		zombiechild->prevsibling = NULL;

	thepid = zombie->pid;

	// It is safe to access these clocks directly as the child process is no
	// longer running at this point and the values are nicely frozen.
	child_execute_clock.Advance(zombie->child_execute_clock.current_time);
	child_system_clock.Advance(zombie->child_system_clock.current_time);

	int status = zombie->exitstatus;
	if ( status < 0 )
		status = W_EXITCODE(128 + SIGKILL, SIGKILL);

	kthread_mutex_lock(&zombie->groupparentlock);
	bool in_limbo = zombie->groupfirst && (zombie->grouplimbo = true);
	kthread_mutex_unlock(&zombie->groupparentlock);

	// And so, the process was fully deleted.
	if ( !in_limbo )
		delete zombie;

	if ( user_status && !CopyToUser(user_status, &status, sizeof(status)) )
		return -1;

	return thepid;
}

static pid_t sys_waitpid(pid_t pid, int* user_status, int options)
{
	return CurrentProcess()->Wait(pid, user_status, options);
}

void Process::Exit(int status)
{
	ScopedLock lock(&threadlock);
	// Status codes can only contain 8 bits according to ISO C and POSIX.
	if ( exitstatus == -1 )
		exitstatus = W_EXITCODE(status & 0xFF, 0);

	// Broadcast SIGKILL to all our threads which will begin our long path
	// of process termination. We simply can't stop the threads as they may
	// be running in kernel mode doing dangerous stuff. This thread will be
	// destroyed by SIGKILL once the system call returns.
	for ( Thread* t = firstthread; t; t = t->nextsibling )
		t->DeliverSignal(SIGKILL);
}

static int sys_exit(int status)
{
	CurrentProcess()->Exit(status);
	return 0;
}

bool Process::DeliverSignal(int signum)
{
	// TODO: How to handle signals that kill the process?
	if ( firstthread )
		return firstthread->DeliverSignal(signum);
	return errno = EINIT, false;
}

bool Process::DeliverGroupSignal(int signum)
{
	ScopedLock lock(&groupparentlock);
	if ( !groupfirst )
		return errno = ESRCH, false;
	for ( Process* iter = groupfirst; iter; iter = iter->groupnext )
		iter->DeliverSignal(signum);
	return true;
}

void Process::AddChildProcess(Process* child)
{
	ScopedLock mylock(&childlock);
	ScopedLock itslock(&child->parentlock);
	assert(!child->parent);
	assert(!child->nextsibling);
	assert(!child->prevsibling);
	child->parent = this;
	child->nextsibling = firstchild;
	child->prevsibling = NULL;
	if ( firstchild )
		firstchild->prevsibling = child;
	firstchild = child;
}

Ref<MountTable> Process::GetMTable()
{
	ScopedLock lock(&ptrlock);
	assert(mtable);
	return mtable;
}

Ref<DescriptorTable> Process::GetDTable()
{
	ScopedLock lock(&ptrlock);
	assert(dtable);
	return dtable;
}

Ref<Descriptor> Process::GetRoot()
{
	ScopedLock lock(&ptrlock);
	assert(root);
	return root;
}

Ref<Descriptor> Process::GetCWD()
{
	ScopedLock lock(&ptrlock);
	assert(cwd);
	return cwd;
}

void Process::SetRoot(Ref<Descriptor> newroot)
{
	ScopedLock lock(&ptrlock);
	assert(newroot);
	root = newroot;
}

void Process::SetCWD(Ref<Descriptor> newcwd)
{
	ScopedLock lock(&ptrlock);
	assert(newcwd);
	cwd = newcwd;
}

Ref<Descriptor> Process::GetDescriptor(int fd)
{
	ScopedLock lock(&ptrlock);
	assert(dtable);
	return dtable->Get(fd);
}

Process* Process::Fork()
{
	assert(CurrentProcess() == this);

	// TODO: This adds the new process to the process table, but it's not ready
	//       and functions that access this new process will be surprised that
	//       it's not fully constructed and really bad things will happen.
	Process* clone = new Process;
	if ( !clone )
		return NULL;

	struct segment* clone_segments = NULL;

	// Fork the segment list.
	if ( segments )
	{
		size_t segments_size = sizeof(struct segment) * segments_used;
		if ( !(clone_segments = (struct segment*) malloc(segments_size)) )
		{
			delete clone;
			return NULL;
		}
		memcpy(clone_segments, segments, segments_size);
	}

	// Fork address-space here and copy memory.
	clone->addrspace = Memory::Fork();
	if ( !clone->addrspace )
	{
		free(clone_segments);
		delete clone;
		return NULL;
	}

	// Now it's too late to clean up here, if anything goes wrong, we simply
	// ask the process to commit suicide before it goes live.
	clone->segments = clone_segments;
	clone->segments_used = segments_used;
	clone->segments_length = segments_used;

	// Remember the relation to the child process.
	AddChildProcess(clone);

	// Add the new process to the current process group.
	kthread_mutex_lock(&groupchildlock);
	kthread_mutex_lock(&group->groupparentlock);
	clone->group = group;
	clone->groupprev = NULL;
	if ( (clone->groupnext = group->groupfirst) )
		group->groupfirst->groupprev = clone;
	group->groupfirst = clone;
	kthread_mutex_unlock(&group->groupparentlock);
	kthread_mutex_unlock(&groupchildlock);

	// Initialize everything that is safe and can't fail.
	kthread_mutex_lock(&resource_limits_lock);
	for ( size_t i = 0; i < RLIMIT_NUM_DECLARED; i++ )
		clone->resource_limits[i] = resource_limits[i];
	kthread_mutex_unlock(&resource_limits_lock);

	kthread_mutex_lock(&nicelock);
	clone->nice = nice;
	kthread_mutex_unlock(&nicelock);

	kthread_mutex_lock(&ptrlock);
	clone->root = root;
	clone->cwd = cwd;
	kthread_mutex_unlock(&ptrlock);

	// Initialize things that can fail and abort if needed.
	bool failure = false;

	kthread_mutex_lock(&ptrlock);
	if ( !(clone->dtable = dtable->Fork()) )
		failure = true;
	//if ( !(clone->mtable = mtable->Fork()) )
	//	failure = true;
	clone->mtable = mtable;
	kthread_mutex_unlock(&ptrlock);

	kthread_mutex_lock(&idlock);
	clone->uid = uid;
	clone->gid = gid;
	clone->euid = euid;
	clone->egid = egid;
	clone->umask = umask;
	kthread_mutex_unlock(&idlock);

	if ( !(clone->program_image_path = String::Clone(program_image_path)) )
		failure = false;

	if ( string_table && (clone->string_table = new char[string_table_length]) )
	{
		memcpy(clone->string_table, string_table, string_table_length);
		clone->string_table_length = string_table_length;
	}

	if ( clone->string_table && symbol_table &&
	     (clone->symbol_table = new Symbol[symbol_table_length]) )
	{
		for ( size_t i = 0; i < symbol_table_length; i++ )
		{
			clone->symbol_table[i].address = symbol_table[i].address;
			clone->symbol_table[i].size = symbol_table[i].size;
			clone->symbol_table[i].name =
				(const char*)((uintptr_t) symbol_table[i].name -
				              (uintptr_t) string_table +
				              (uintptr_t) clone->string_table);
		}
		clone->symbol_table_length = symbol_table_length;
	}

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
	string_table_length = 0;
	symbol_table_length = 0;
	delete[] string_table; string_table = NULL;
	delete[] symbol_table; symbol_table = NULL;

	DeleteTimers();

	ResetAddressSpace();
}

int Process::Execute(const char* programname, const uint8_t* program,
                     size_t programsize, int argc, const char* const* argv,
                     int envc, const char* const* envp,
                     CPU::InterruptRegisters* regs)
{
	assert(CurrentProcess() == this);

	char* programname_clone = String::Clone(programname);
	if ( !programname_clone )
		return -1;

	ELF::Auxiliary aux;

	addr_t entry = ELF::Construct(CurrentProcess(), program, programsize, &aux);
	if ( !entry ) { delete[] programname_clone; return -1; }

	delete[] program_image_path;
	program_image_path = programname_clone; programname_clone = NULL;

	uintptr_t userspace_addr;
	size_t userspace_size;
	Memory::GetUserVirtualArea(&userspace_addr, &userspace_size);

	const size_t DEFAULT_STACK_SIZE = 512UL * 1024UL;
	void* const PREFERRED_STACK_LOCATION =
		(void*) (userspace_addr + userspace_size - DEFAULT_STACK_SIZE);
	const int STACK_PROTECTION = PROT_READ | PROT_WRITE | PROT_KREAD |
	                             PROT_KWRITE | PROT_FORK;

	// Attempt to allocate a stack for the new process.
	kthread_mutex_lock(&segment_lock);
	struct segment stack_segment;
	if ( !PlaceSegment(&stack_segment, this, PREFERRED_STACK_LOCATION, DEFAULT_STACK_SIZE, 0) ||
	     !Memory::MapMemory(this, stack_segment.addr, stack_segment.size, stack_segment.prot = STACK_PROTECTION) )
	{
		kthread_mutex_unlock(&segment_lock);
		ResetForExecute();
		return errno = ENOMEM, -1;
	}
	kthread_mutex_unlock(&segment_lock);

	addr_t stackpos = stack_segment.addr + stack_segment.size;

	// Alright, move argv onto the new stack! First figure out exactly how
	// big argv actually is.
	addr_t argvpos = stackpos - sizeof(char*) * (argc+1);
	char** stackargv = (char**) argvpos;

	size_t argvsize = 0;
	for ( int i = 0; i < argc; i++ )
	{
		size_t len = strlen(argv[i]) + 1;
		argvsize += len;
		char* dest = ((char*) argvpos) - argvsize;
		stackargv[i] = dest;
		memcpy(dest, argv[i], len);
	}

	stackargv[argc] = NULL;

	if ( argvsize % 16UL )
		argvsize += 16 - argvsize % 16UL;

	// And then move envp onto the stack.
	addr_t envppos = argvpos - argvsize - sizeof(char*) * (envc+1);
	char** stackenvp = (char**) envppos;

	size_t envpsize = 0;
	for ( int i = 0; i < envc; i++ )
	{
		size_t len = strlen(envp[i]) + 1;
		envpsize += len;
		char* dest = ((char*) envppos) - envpsize;
		stackenvp[i] = dest;
		memcpy(dest, envp[i], len);
	}

	stackenvp[envc] = NULL;

	if ( envpsize % 16UL )
		envpsize += 16 - envpsize % 16UL;

	stackpos = envppos - envpsize;

	dtable->OnExecute();

	ExecuteCPU(argc, stackargv, envc, stackenvp, stackpos, entry, regs);

	return 0;
}

static
int sys_execve_kernel(const char* filename,
                      int argc,
                      char* const argv[],
                      int envc,
                      char* const envp[],
                      CPU::InterruptRegisters* regs)
{
	Process* process = CurrentProcess();

	ioctx_t ctx;
	SetupKernelIOCtx(&ctx);
	Ref<Descriptor> from = filename[0] == '/' ? process->GetRoot() : process->GetCWD();
	Ref<Descriptor> desc = from->open(&ctx, filename, O_EXEC | O_READ, 0);
	if ( !desc )
		return -1;

	struct stat st;
	if ( desc->stat(&ctx, &st) )
		return -1;
	if ( st.st_size < 0 )
		return errno = EINVAL, -1;
	if ( (uintmax_t) SIZE_MAX < (uintmax_t) st.st_size )
		return errno = EFBIG, -1;

	size_t filesize = (size_t) st.st_size;
	uint8_t* buffer = new uint8_t[filesize];
	if ( !buffer )
		return -1;

	for ( size_t sofar = 0; sofar < filesize; )
	{
		ssize_t amount = desc->read(&ctx, buffer + sofar, filesize - sofar);
		if ( amount < 0 )
			return delete[] buffer, -1;
		if ( amount == 0 )
			return delete[] buffer, errno = EEOF, -1;
		sofar += amount;
	}

	int result = process->Execute(filename, buffer, filesize, argc, argv, envc, envp, regs);

	delete[] buffer;

	return result;
}

static
int sys_execve(const char* user_filename,
               char* const user_argv[],
               char* const user_envp[])
{
	char* filename;
	int argc;
	int envc;
	char** argv;
	char** envp;
	int result = -1;
	CPU::InterruptRegisters regs;
	memset(&regs, 0, sizeof(regs));

	if ( !user_filename || !user_argv || !user_envp )
		return errno = EFAULT, -1;

	if ( !(filename = GetStringFromUser(user_filename)) )
		goto cleanup_done;

	argc = 0;
	while ( true )
	{
		const char* user_arg;
		if ( !CopyFromUser(&user_arg, user_argv + argc, sizeof(user_arg)) )
			goto cleanup_filename;
		if ( !user_arg )
			break;
		if ( ++argc == INT_MAX )
		{
			errno = E2BIG;
			goto cleanup_filename;
		}
	}

	argv = new char*[argc+1];
	if ( !argv )
		goto cleanup_filename;
	memset(argv, 0, sizeof(char*) * (argc+1));

	for ( int i = 0; i < argc; i++ )
	{
		const char* user_arg;
		if ( !CopyFromUser(&user_arg, user_argv + i, sizeof(user_arg)) )
			goto cleanup_argv;
		if ( !(argv[i] = GetStringFromUser(user_arg)) )
			goto cleanup_argv;
	}

	envc = 0;
	while ( true )
	{
		const char* user_env;
		if ( !CopyFromUser(&user_env, user_envp + envc, sizeof(user_env)) )
			goto cleanup_argv;
		if ( !user_env )
			break;
		if ( ++envc == INT_MAX  )
		{
			errno = E2BIG;
			goto cleanup_argv;
		}
	}

	envp = new char*[envc+1];
	if ( !envp )
		goto cleanup_argv;
	memset(envp, 0, sizeof(char*) * (envc+1));

	for ( int i = 0; i < envc; i++ )
	{
		const char* user_env;
		if ( !CopyFromUser(&user_env, user_envp + i, sizeof(user_env)) )
			goto cleanup_envp;
		if ( !(envp[i] = GetStringFromUser(user_envp[i])) )
			goto cleanup_envp;
	}

	result = sys_execve_kernel(filename, argc, argv, envc, envp, &regs);

cleanup_envp:
	for ( int i = 0; i < envc; i++)
		delete[] envp[i];
	delete[] envp;
cleanup_argv:
	for ( int i = 0; i < argc; i++)
		delete[] argv[i];
	delete[] argv;
cleanup_filename:
	delete[] filename;
cleanup_done:
	if ( result == 0 )
		CPU::LoadRegisters(&regs);
	return result;
}

static pid_t sys_tfork(int flags, tforkregs_t* user_regs)
{
	tforkregs_t regs;
	if ( !CopyFromUser(&regs, user_regs, sizeof(regs)) )
		return -1;

	if ( Signal::IsPending() )
		return errno = EINTR, -1;

	// TODO: Properly support tfork(2).
	if ( flags != SFFORK )
		return errno = ENOSYS, -1;

	CPU::InterruptRegisters cpuregs;
	InitializeThreadRegisters(&cpuregs, &regs);

	// TODO: Is it a hack to create a new kernel stack here?
	Thread* curthread = CurrentThread();
	uint8_t* newkernelstack = new uint8_t[curthread->kernelstacksize];
	if ( !newkernelstack )
		return -1;

	Process* clone = CurrentProcess()->Fork();
	if ( !clone ) { delete[] newkernelstack; return -1; }

	// If the thread could not be created, make the process commit suicide
	// in a manner such that we don't wait for its zombie.
	Thread* thread = CreateKernelThread(clone, &cpuregs, regs.fsbase,
	                                    regs.gsbase);
	if ( !thread )
	{
		clone->AbortConstruction();
		return -1;
	}

	thread->kernelstackpos = (addr_t) newkernelstack;
	thread->kernelstacksize = curthread->kernelstacksize;
	thread->kernelstackmalloced = true;
	thread->sighandler = curthread->sighandler;

	StartKernelThread(thread);

	return clone->pid;
}

static pid_t sys_getpid()
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

static pid_t sys_getppid()
{
	return CurrentProcess()->GetParentProcessId();
}

static pid_t sys_getpgid(pid_t pid)
{
	Process* process = !pid ? CurrentProcess() : Process::Get(pid);
	if ( !process )
		return errno = ESRCH, -1;

	// Prevent the process group from changing while we read it.
	ScopedLock childlock(&process->groupchildlock);
	assert(process->group);

	return process->group->pid;
}

static int sys_setpgid(pid_t pid, pid_t pgid)
{
	// TODO: Prevent changing the process group of zombies and other volatile
	//       things that are about to implode.
	// TODO: Either prevent changing the process group after an exec or provide
	//       a version of this system call with a flags parameter that lets you
	//       decide if you want this behavior. This will fix a race condition
	//       where the shell spawns a child and both parent and child sets the
	//       process group, but the child sets the process group and execve's
	//       and the new program image exploits this 'bug' and also changes the
	//       process group, and then the shell gets around to change the process
	//       group. This probably unlikely, but correctness over all!

	// Find the processes in question.
	Process* process = !pid ? CurrentProcess() : Process::Get(pid);
	if ( !process )
		return errno = ESRCH, -1;
	Process* group = !pgid ? process : Process::Get(pgid);
	if ( !group )
		return errno = ESRCH, -1;

	// Prevent the current group from being changed while we also change it
	ScopedLock childlock(&process->groupchildlock);
	assert(process->group);

	// Exit early if this is a noop.
	if ( process->group == group )
		return 0;

	// Prevent changing the process group of a process group leader.
	if ( process->group == process )
		return errno = EPERM, -1;

	// Remove the process from its current process group.
	kthread_mutex_lock(&process->group->groupparentlock);
	if ( process->groupprev )
		process->groupprev->groupnext = process->groupnext;
	else
		process->group->groupfirst = process->groupnext;
	if ( process->groupnext )
		process->groupnext->groupprev = process->groupprev;
	kthread_cond_signal(&process->group->groupchildleft);
	kthread_mutex_unlock(&process->group->groupparentlock);
	process->group->NotifyLeftProcessGroup();
	process->group = NULL;

	// TODO: Somehow prevent joining a zombie group, or worse yet, one that is
	//       currently being deleted by its parent!

	// Insert the process into its new process group.
	kthread_mutex_lock(&group->groupparentlock);
	process->groupprev = NULL;
	process->groupnext = group->groupfirst;
	if ( group->groupfirst )
		group->groupfirst->groupprev = process;
	group->groupfirst = process;
	process->group = group;
	kthread_mutex_unlock(&group->groupparentlock);

	return 0;
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
	if ( a->pid < b->pid )
		return -1;
	if ( a->pid > b->pid )
		return 1;
	return 0;
}

int ProcessPIDCompare(Process* a, pid_t pid)
{
	if ( a->pid < pid )
		return -1;
	if ( a->pid > pid )
		return 1;
	return 0;
}

SortedList<Process*>* pidlist;

Process* Process::Get(pid_t pid)
{
	ScopedLock lock(&pidalloclock);
	size_t index = pidlist->Search(ProcessPIDCompare, pid);
	if ( index == SIZE_MAX )
		return NULL;

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
	assert(index != SIZE_MAX);

	pidlist->Remove(index);
}

static void* sys_sbrk(intptr_t increment)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->segment_lock);

	// Locate the heap segment.
	struct segment* heap_segment = NULL;
	for ( size_t i = process->segments_used; !heap_segment && i != 0; i-- )
	{
		struct segment* candidate = &process->segments[i-1];
		if ( !(candidate->prot & PROT_HEAP) )
			continue;
		heap_segment = candidate;
	}
	if ( !heap_segment )
		return errno = ENOMEM, (void*) -1UL;

	assert(IsUserspaceSegment(heap_segment));

	// Decrease the size of the heap segment if requested.
	if ( increment < 0 )
	{
		uintptr_t abs_amount = Page::AlignDown(- (uintptr_t) increment);
		if ( heap_segment->size < abs_amount )
			abs_amount = heap_segment->size;
		uintptr_t new_end = heap_segment->addr + heap_segment->size - abs_amount;
		Memory::UnmapRange(new_end, abs_amount);
		heap_segment->size -= abs_amount;
		// TODO: How do we handle that the heap shrinks to 0 bytes?
	}

	// Increase the size of the heap if requested.
	if ( 0 < increment )
	{
		uintptr_t abs_amount = Page::AlignUp(increment);
		uintptr_t max_growth = 0 - (heap_segment->addr + heap_segment->size);
		if ( max_growth < abs_amount )
			return errno = ENOMEM, (void*) -1UL;
		struct segment growth;
		growth.addr = heap_segment->addr + heap_segment->size;
		growth.size = abs_amount;
		growth.prot = heap_segment->prot;
		if ( !IsUserspaceSegment(&growth) )
			return errno = ENOMEM, (void*) -1UL;
		if ( FindOverlappingSegment(process, &growth) )
			return errno = ENOMEM, (void*) -1UL;
		if ( !Memory::MapRange(growth.addr, growth.size, growth.prot) )
			return errno = ENOMEM, (void*) -1UL;
		heap_segment->size += growth.size;
	}

	assert(IsUserspaceSegment(heap_segment));

	return (void*) (heap_segment->addr + heap_segment->size);
}

static size_t sys_getpagesize()
{
	return Page::Size();
}

static mode_t sys_umask(mode_t newmask)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	mode_t oldmask = process->umask;
	process->umask = newmask & 0666;
	return oldmask;
}

static mode_t sys_getumask(void)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->umask;
}

void Process::Init()
{
	Syscall::Register(SYSCALL_EXECVE, (void*) sys_execve);
	Syscall::Register(SYSCALL_EXIT, (void*) sys_exit);
	Syscall::Register(SYSCALL_GETPAGESIZE, (void*) sys_getpagesize);
	Syscall::Register(SYSCALL_GETPGID, (void*) sys_getpgid);
	Syscall::Register(SYSCALL_GETPID, (void*) sys_getpid);
	Syscall::Register(SYSCALL_GETPPID, (void*) sys_getppid);
	Syscall::Register(SYSCALL_GETUMASK, (void*) sys_getumask);
	Syscall::Register(SYSCALL_SBRK, (void*) sys_sbrk);
	Syscall::Register(SYSCALL_SETPGID, (void*) sys_setpgid);
	Syscall::Register(SYSCALL_TFORK, (void*) sys_tfork);
	Syscall::Register(SYSCALL_UMASK, (void*) sys_umask);
	Syscall::Register(SYSCALL_WAITPID, (void*) sys_waitpid);

	pidalloclock = KTHREAD_MUTEX_INITIALIZER;
	nextpidtoallocate = 0;

	pidlist = new SortedList<Process*>(ProcessCompare);
	if ( !pidlist )
		Panic("could not allocate pidlist\n");
}

} // namespace Sortix
