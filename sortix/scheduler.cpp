/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    scheduler.cpp
    Decides the order to execute threads in and switching between them.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <string.h>

#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/timespec.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/time.h>

#include "x86-family/gdt.h"
#include "x86-family/float.h"
#include "thread.h"
#include "process.h"
#include "signal.h"
#include "scheduler.h"

namespace Sortix {
namespace Scheduler {

const uint32_t SCHED_MAGIC = 0x1234567;

volatile unsigned long premagic;
static Thread* currentthread;
} // namespace Scheduler
Thread* CurrentThread() { return Scheduler::currentthread; }
Process* CurrentProcess() { return CurrentThread()->process; }
namespace Scheduler {

uint8_t dummythreaddata[sizeof(Thread)] __attribute__ ((aligned (8)));
Thread* dummythread;
Thread* idlethread;
Thread* firstrunnablethread;
Thread* firstsleepingthread;
Process* initprocess;
volatile unsigned long postmagic;

static inline void SetCurrentThread(Thread* newcurrentthread)
{
	currentthread = newcurrentthread;
}

void LogBeginSwitch(Thread* current, const CPU::InterruptRegisters* regs);
void LogSwitch(Thread* current, Thread* next);
void LogEndSwitch(Thread* current, const CPU::InterruptRegisters* regs);

static Thread* PopNextThread()
{
	if ( !firstrunnablethread ) { return idlethread; }
	Thread* result = firstrunnablethread;
	firstrunnablethread = firstrunnablethread->schedulerlistnext;
	return result;
}

static Thread* ValidatedPopNextThread()
{
	assert(premagic == SCHED_MAGIC);
	assert(postmagic == SCHED_MAGIC);
	Thread* nextthread = PopNextThread();
	if ( !nextthread ) { Panic("Had no thread to switch to."); }
	if ( nextthread->terminated )
	{
		PanicF("Running a terminated thread 0x%p", nextthread);
	}
	addr_t newaddrspace = nextthread->process->addrspace;
	if ( !Page::IsAligned(newaddrspace) )
	{
		PanicF("Thread 0x%p, process %i (0x%p) (backup: %i), had bad "
		       "address space variable: 0x%zx: not page-aligned "
		       "(backup: 0x%zx)\n", nextthread,
		       nextthread->process->pid, nextthread->process,
		       -1/*nextthread->pidbackup*/, newaddrspace,
		       (addr_t)-1 /*nextthread->addrspacebackup*/);
	}
	return nextthread;
}

static void DoActualSwitch(CPU::InterruptRegisters* regs)
{
	Thread* current = CurrentThread();
	LogBeginSwitch(current, regs);

	Thread* next = ValidatedPopNextThread();
	LogSwitch(current, next);

	if ( current == next ) { return; }

	current->SaveRegisters(regs);
	next->LoadRegisters(regs);

	Float::NotityTaskSwitch();

	addr_t newaddrspace = next->addrspace;
	Memory::SwitchAddressSpace(newaddrspace);
	SetCurrentThread(next);

	addr_t stacklower = next->kernelstackpos;
	size_t stacksize = next->kernelstacksize;
	addr_t stackhigher = stacklower + stacksize;
	assert(stacklower && stacksize && stackhigher);
	GDT::SetKernelStack(stacklower, stacksize, stackhigher);

	LogEndSwitch(next, regs);
}

void Switch(CPU::InterruptRegisters* regs)
{
	assert(premagic == SCHED_MAGIC);
	assert(postmagic == SCHED_MAGIC);
	DoActualSwitch(regs);
	if ( regs->signal_pending && regs->InUserspace() )
		Signal::Dispatch(regs);
	assert(premagic == SCHED_MAGIC);
	assert(postmagic == SCHED_MAGIC);
}

const bool DEBUG_BEGINCTXSWITCH = false;
const bool DEBUG_CTXSWITCH = false;
const bool DEBUG_ENDCTXSWITCH = false;

void LogBeginSwitch(Thread* current, const CPU::InterruptRegisters* regs)
{
	bool alwaysdebug = false;
	bool isidlethread = current == idlethread;
	bool dodebug = DEBUG_BEGINCTXSWITCH && !isidlethread;
	if ( alwaysdebug || dodebug )
	{
		Log::PrintF("Switching from 0x%p", current);
		regs->LogRegisters();
		Log::Print("\n");
	}
}

void LogSwitch(Thread* current, Thread* next)
{
	bool alwaysdebug = false;
	bool different = current == idlethread;
	bool dodebug = DEBUG_CTXSWITCH && different;
	if ( alwaysdebug || dodebug )
	{
		Log::PrintF("switching from %u:%u (0x%p) to %u:%u (0x%p) \n",
		            current->process->pid, 0, current,
		            next->process->pid, 0, next);
	}
}

void LogEndSwitch(Thread* current, const CPU::InterruptRegisters* regs)
{
	bool alwaysdebug = false;
	bool isidlethread = current == idlethread;
	bool dodebug = DEBUG_BEGINCTXSWITCH && !isidlethread;
	if ( alwaysdebug || dodebug )
	{
		Log::PrintF("Switched to 0x%p", current);
		regs->LogRegisters();
		Log::Print("\n");
	}
}

static void InterruptYieldCPU(CPU::InterruptRegisters* regs, void* /*user*/)
{
	Switch(regs);
}

static void ThreadExitCPU(CPU::InterruptRegisters* regs, void* /*user*/)
{
	// Can't use floating point instructions from now.
	Float::NofityTaskExit(currentthread);
	SetThreadState(currentthread, Thread::State::DEAD);
	InterruptYieldCPU(regs, NULL);
}

// The idle thread serves no purpose except being an infinite loop that does
// nothing, which is only run when the system has nothing to do.
void SetIdleThread(Thread* thread)
{
	assert(!idlethread);
	idlethread = thread;
	SetThreadState(thread, Thread::State::NONE);
	SetCurrentThread(thread);
}

void SetDummyThreadOwner(Process* process)
{
	dummythread->process = process;
}

void SetInitProcess(Process* init)
{
	initprocess = init;
}

Process* GetInitProcess()
{
	return initprocess;
}

void SetThreadState(Thread* thread, Thread::State state)
{
	bool wasenabled = Interrupt::SetEnabled(false);

	// Remove the thread from the list of runnable threads.
	if ( thread->state == Thread::State::RUNNABLE &&
	     state != Thread::State::RUNNABLE )
	{
		if ( thread == firstrunnablethread ) { firstrunnablethread = thread->schedulerlistnext; }
		if ( thread == firstrunnablethread ) { firstrunnablethread = NULL; }
		assert(thread->schedulerlistprev);
		assert(thread->schedulerlistnext);
		thread->schedulerlistprev->schedulerlistnext = thread->schedulerlistnext;
		thread->schedulerlistnext->schedulerlistprev = thread->schedulerlistprev;
		thread->schedulerlistprev = NULL;
		thread->schedulerlistnext = NULL;
	}

	// Insert the thread into the scheduler's carousel linked list.
	if ( thread->state != Thread::State::RUNNABLE &&
	     state == Thread::State::RUNNABLE )
	{
		if ( firstrunnablethread == NULL ) { firstrunnablethread = thread; }
		thread->schedulerlistprev = firstrunnablethread->schedulerlistprev;
		thread->schedulerlistnext = firstrunnablethread;
		firstrunnablethread->schedulerlistprev = thread;
		thread->schedulerlistprev->schedulerlistnext = thread;
	}

	thread->state = state;

	assert(thread->state != Thread::State::RUNNABLE || thread->schedulerlistprev);
	assert(thread->state != Thread::State::RUNNABLE || thread->schedulerlistnext);

	Interrupt::SetEnabled(wasenabled);
}

Thread::State GetThreadState(Thread* thread)
{
	return thread->state;
}

static void SleepUntil(struct timespec wakeat)
{
	while ( timespec_lt(Time::Get(CLOCK_BOOT), wakeat) )
		Yield();
}

int sys_sleep(size_t secs)
{
	struct timespec delay = timespec_make(secs, 0);
	struct timespec now = Time::Get(CLOCK_BOOT);
	SleepUntil(timespec_add(now, delay));
	return 0;
}

int sys_usleep(size_t usecs)
{
	size_t secs = usecs / 1000000;
	size_t nsecs = (usecs % 1000000) * 1000;
	struct timespec delay = timespec_make(secs, nsecs);
	struct timespec now = Time::Get(CLOCK_BOOT);
	SleepUntil(timespec_add(now, delay));
	return 0;
}

extern "C" void yield_cpu_handler();
extern "C" void thread_exit_handler();

void Init()
{
	premagic = postmagic = SCHED_MAGIC;

	// We use a dummy so that the first context switch won't crash when the
	// current thread is accessed. This lets us avoid checking whether it is
	// NULL (which it only will be once), which gives simpler code.
	dummythread = (Thread*) &dummythreaddata;
	memset(dummythread, 0, sizeof(*dummythread));
	dummythread->schedulerlistprev = dummythread;
	dummythread->schedulerlistnext = dummythread;
	currentthread = dummythread;
	firstrunnablethread = NULL;
	firstsleepingthread = NULL;
	idlethread = NULL;

	// Register our raw handler with user-space access. It calls our real
	// handler after common interrupt preparation stuff has occured.
	Interrupt::RegisterRawHandler(129, yield_cpu_handler, true);
	Interrupt::RegisterHandler(129, InterruptYieldCPU, NULL);
	Interrupt::RegisterRawHandler(132, thread_exit_handler, true);
	Interrupt::RegisterHandler(132, ThreadExitCPU, NULL);

	Syscall::Register(SYSCALL_SLEEP, (void*) sys_sleep);
	Syscall::Register(SYSCALL_USLEEP, (void*) sys_usleep);
}

} // namespace Scheduler
} // namespace Sortix
