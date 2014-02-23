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

    scheduler.cpp
    Decides the order to execute threads in and switching between them.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <msr.h>
#include <string.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/timespec.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>

#if defined(__i386__) || defined(__x86_64__)
#include "x86-family/gdt.h"
#include "x86-family/float.h"
#endif

namespace Sortix {
namespace Scheduler {

static Thread* current_thread;
static Thread* idle_thread;
static Thread* first_runnable_thread;
static Thread* first_sleeping_thread;
static Process* init_process;

static Thread* PopNextThread()
{
	if ( first_runnable_thread )
	{
		Thread* result = first_runnable_thread;
		first_runnable_thread = first_runnable_thread->scheduler_list_next;
		return result;
	}

	return idle_thread;
}

static void DoActualSwitch(CPU::InterruptRegisters* regs)
{
	Thread* prev = CurrentThread();
	Thread* next = PopNextThread();

	if ( prev == next )
		return;

	prev->SaveRegisters(regs);
	next->LoadRegisters(regs);

	Memory::SwitchAddressSpace(next->addrspace);
	current_thread = next;

#if defined(__i386__) || defined(__x86_64__)
	Float::NotityTaskSwitch();
	GDT::SetKernelStack(next->kernelstackpos, next->kernelstacksize,
	                    next->kernelstackpos + next->kernelstacksize);
#endif

#if defined(__i386__)
	prev->fsbase = (unsigned long) GDT::GetFSBase();
	prev->gsbase = (unsigned long) GDT::GetGSBase();
	GDT::SetFSBase((uint32_t) next->fsbase);
	GDT::SetGSBase((uint32_t) next->gsbase);
#elif defined(__x86_64__)
	prev->fsbase = (unsigned long) rdmsr(MSRID_FSBASE);
	prev->gsbase = (unsigned long) rdmsr(MSRID_GSBASE);
	wrmsr(MSRID_FSBASE, (uint64_t) next->fsbase);
	wrmsr(MSRID_GSBASE, (uint64_t) next->gsbase);
#endif
}

void Switch(CPU::InterruptRegisters* regs)
{
	DoActualSwitch(regs);

	if ( regs->signal_pending && regs->InUserspace() )
	{
		Interrupt::Enable();
		Signal::DispatchHandler(regs, NULL);
	}
}

void InterruptYieldCPU(CPU::InterruptRegisters* regs, void* /*user*/)
{
	Switch(regs);
}

void ThreadExitCPU(CPU::InterruptRegisters* regs, void* /*user*/)
{
#if defined(__i386__) || defined(__x86_64__)
	// Can't use floating point instructions from now.
	Float::NofityTaskExit(current_thread);
#endif
	SetThreadState(current_thread, ThreadState::DEAD);
	InterruptYieldCPU(regs, NULL);
}

// The idle thread serves no purpose except being an infinite loop that does
// nothing, which is only run when the system has nothing to do.
void SetIdleThread(Thread* thread)
{
	assert(!idle_thread);
	idle_thread = thread;
	SetThreadState(thread, ThreadState::NONE);
	current_thread = thread;
}

void SetInitProcess(Process* init)
{
	init_process = init;
}

Process* GetInitProcess()
{
	return init_process;
}

Process* GetKernelProcess()
{
	return idle_thread->process;
}

void SetThreadState(Thread* thread, ThreadState state)
{
	bool wasenabled = Interrupt::SetEnabled(false);

	// Remove the thread from the list of runnable threads.
	if ( thread->state == ThreadState::RUNNABLE &&
	     state != ThreadState::RUNNABLE )
	{
		if ( thread == first_runnable_thread )
			first_runnable_thread = thread->scheduler_list_next;
		if ( thread == first_runnable_thread )
			first_runnable_thread = NULL;
		assert(thread->scheduler_list_prev);
		assert(thread->scheduler_list_next);
		thread->scheduler_list_prev->scheduler_list_next = thread->scheduler_list_next;
		thread->scheduler_list_next->scheduler_list_prev = thread->scheduler_list_prev;
		thread->scheduler_list_prev = NULL;
		thread->scheduler_list_next = NULL;
	}

	// Insert the thread into the scheduler's carousel linked list.
	if ( thread->state != ThreadState::RUNNABLE &&
	     state == ThreadState::RUNNABLE )
	{
		if ( first_runnable_thread == NULL )
			first_runnable_thread = thread;
		thread->scheduler_list_prev = first_runnable_thread->scheduler_list_prev;
		thread->scheduler_list_next = first_runnable_thread;
		first_runnable_thread->scheduler_list_prev = thread;
		thread->scheduler_list_prev->scheduler_list_next = thread;
	}

	thread->state = state;

	assert(thread->state != ThreadState::RUNNABLE || thread->scheduler_list_prev);
	assert(thread->state != ThreadState::RUNNABLE || thread->scheduler_list_next);

	Interrupt::SetEnabled(wasenabled);
}

ThreadState GetThreadState(Thread* thread)
{
	return thread->state;
}

static int sys_sched_yield(void)
{
	return kthread_yield(), 0;
}

void Init()
{
	first_runnable_thread = NULL;
	first_sleeping_thread = NULL;
	idle_thread = NULL;

	Syscall::Register(SYSCALL_SCHED_YIELD, (void*) sys_sched_yield);
}

} // namespace Scheduler
} // namespace Sortix

namespace Sortix {

Thread* CurrentThread()
{
	return Scheduler::current_thread;
}

Process* CurrentProcess()
{
	return CurrentThread()->process;
}

} // namespace Sortix
