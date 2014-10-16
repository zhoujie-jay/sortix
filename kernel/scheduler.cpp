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
#include <string.h>
#include <timespec.h>

#if defined(__x86_64__)
#include <msr.h>
#endif

#include <sortix/clock.h>
#include <sortix/timespec.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/registers.h>
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

void SaveInterruptedContext(const struct interrupt_context* intctx,
                            struct thread_registers* registers)
{
#if defined(__i386__)
	registers->signal_pending = intctx->signal_pending;
	registers->kerrno = intctx->kerrno;
	registers->eax = intctx->eax;
	registers->ebx = intctx->ebx;
	registers->ecx = intctx->ecx;
	registers->edx = intctx->edx;
	registers->edi = intctx->edi;
	registers->esi = intctx->esi;
	registers->esp = intctx->esp;
	registers->ebp = intctx->ebp;
	registers->eip = intctx->eip;
	registers->eflags = intctx->eflags;
	registers->fsbase = (unsigned long) GDT::GetFSBase();
	registers->gsbase = (unsigned long) GDT::GetGSBase();
	asm ( "mov %%cr3, %0" : "=r"(registers->cr3) );
	registers->kernel_stack = GDT::GetKernelStack();
	registers->cs = intctx->cs;
	registers->ds = intctx->ds;
	registers->ss = intctx->ss;
	asm volatile ("fxsave (%0)" : : "r"(registers->fpuenv));
#elif defined(__x86_64__)
	registers->signal_pending = intctx->signal_pending;
	registers->kerrno = intctx->kerrno;
	registers->rax = intctx->rax;
	registers->rbx = intctx->rbx;
	registers->rcx = intctx->rcx;
	registers->rdx = intctx->rdx;
	registers->rdi = intctx->rdi;
	registers->rsi = intctx->rsi;
	registers->rsp = intctx->rsp;
	registers->rbp = intctx->rbp;
	registers->r8 = intctx->r8;
	registers->r9 = intctx->r9;
	registers->r10 = intctx->r10;
	registers->r11 = intctx->r11;
	registers->r12 = intctx->r12;
	registers->r13 = intctx->r13;
	registers->r14 = intctx->r14;
	registers->r15 = intctx->r15;
	registers->r15 = intctx->r15;
	registers->rip = intctx->rip;
	registers->rflags = intctx->rflags;
	registers->fsbase = (unsigned long) rdmsr(MSRID_FSBASE);
	registers->gsbase = (unsigned long) rdmsr(MSRID_GSBASE);
	asm ( "mov %%cr3, %0" : "=r"(registers->cr3) );
	registers->kernel_stack = GDT::GetKernelStack();
	registers->cs = intctx->cs;
	registers->ds = intctx->ds;
	registers->ss = intctx->ss;
	asm volatile ("fxsave (%0)" : : "r"(registers->fpuenv));
#else
#warning "You need to implement register saving"
#endif
}

void LoadInterruptedContext(struct interrupt_context* intctx,
                            const struct thread_registers* registers)
{
#if defined(__i386__)
	intctx->signal_pending = registers->signal_pending;
	intctx->kerrno = registers->kerrno;
	intctx->eax = registers->eax;
	intctx->ebx = registers->ebx;
	intctx->ecx = registers->ecx;
	intctx->edx = registers->edx;
	intctx->edi = registers->edi;
	intctx->esi = registers->esi;
	intctx->esp = registers->esp;
	intctx->ebp = registers->ebp;
	intctx->eip = registers->eip;
	intctx->eflags = registers->eflags;
	GDT::SetFSBase(registers->fsbase);
	GDT::SetGSBase(registers->gsbase);
	asm volatile ( "mov %0, %%cr3" : : "r"(registers->cr3) );
	GDT::SetKernelStack(registers->kernel_stack);
	intctx->cs = registers->cs;
	intctx->ds = registers->ds;
	intctx->ss = registers->ss;
	asm volatile ("fxrstor (%0)" : : "r"(registers->fpuenv));
#elif defined(__x86_64__)
	intctx->signal_pending = registers->signal_pending;
	intctx->kerrno = registers->kerrno;
	intctx->rax = registers->rax;
	intctx->rbx = registers->rbx;
	intctx->rcx = registers->rcx;
	intctx->rdx = registers->rdx;
	intctx->rdi = registers->rdi;
	intctx->rsi = registers->rsi;
	intctx->rsp = registers->rsp;
	intctx->rbp = registers->rbp;
	intctx->r8 = registers->r8;
	intctx->r9 = registers->r9;
	intctx->r10 = registers->r10;
	intctx->r11 = registers->r11;
	intctx->r12 = registers->r12;
	intctx->r13 = registers->r13;
	intctx->r14 = registers->r14;
	intctx->r15 = registers->r15;
	intctx->r15 = registers->r15;
	intctx->rip = registers->rip;
	intctx->rflags = registers->rflags;
	wrmsr(MSRID_FSBASE, registers->fsbase);
	wrmsr(MSRID_GSBASE, registers->gsbase);
	asm volatile ( "mov %0, %%cr3" : : "r"(registers->cr3) );
	GDT::SetKernelStack(registers->kernel_stack);
	intctx->cs = registers->cs;
	intctx->ds = registers->ds;
	intctx->ss = registers->ss;
	asm volatile ("fxrstor (%0)" : : "r"(registers->fpuenv));
#else
#warning "You need to implement register loading"
#endif
}

static
void SwitchThread(struct interrupt_context* intctx, Thread* prev, Thread* next)
{
	if ( prev == next )
		return;

	SaveInterruptedContext(intctx, &prev->registers);
	if ( !prev->registers.cr3 )
		Log::PrintF("Thread %p had cr3=0x%zx\n", prev, prev->registers.cr3);
	if ( !next->registers.cr3 )
		Log::PrintF("Thread %p has cr3=0x%zx\n", next, next->registers.cr3);
	LoadInterruptedContext(intctx, &next->registers);

	current_thread = next;
}

static Thread* idle_thread;
static Thread* first_runnable_thread;
static Thread* true_current_thread;
static Process* init_process;

static Thread* FindRunnableThreadWithSystemTid(uintptr_t system_tid)
{
	Thread* begun_thread = current_thread;
	Thread* iter = begun_thread;
	do
	{
		if ( iter->system_tid == system_tid )
			return iter;
		iter = iter->scheduler_list_next;
	} while ( iter != begun_thread );
	return NULL;
}

static Thread* PopNextThread(bool yielded)
{
	Thread* result;

	uintptr_t yield_to_tid = current_thread->yield_to_tid;
	if ( yielded && yield_to_tid != 0 )
	{
		if ( (result = FindRunnableThreadWithSystemTid(yield_to_tid)) )
			return result;
	}

	if ( first_runnable_thread )
	{
		result = first_runnable_thread;
		first_runnable_thread = first_runnable_thread->scheduler_list_next;
	}
	else
	{
		result = idle_thread;
	}

	true_current_thread = result;

	return result;
}

static void RealSwitch(struct interrupt_context* intctx, bool yielded)
{
	SwitchThread(intctx, CurrentThread(), PopNextThread(yielded));

	if ( intctx->signal_pending && InUserspace(intctx) )
	{
		Interrupt::Enable();
		Signal::DispatchHandler(intctx, NULL);
	}
}

void Switch(struct interrupt_context* intctx)
{
	RealSwitch(intctx, false);
}

void InterruptYieldCPU(struct interrupt_context* intctx, void* /*user*/)
{
	RealSwitch(intctx, true);
}

void ThreadExitCPU(struct interrupt_context* intctx, void* /*user*/)
{
	SetThreadState(current_thread, ThreadState::DEAD);
	RealSwitch(intctx, false);
}

// The idle thread serves no purpose except being an infinite loop that does
// nothing, which is only run when the system has nothing to do.
void SetIdleThread(Thread* thread)
{
	assert(!idle_thread);
	idle_thread = thread;
	SetThreadState(thread, ThreadState::NONE);
	current_thread = thread;
	true_current_thread = thread;
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

} // namespace Scheduler
} // namespace Sortix

namespace Sortix {

int sys_sched_yield(void)
{
	return kthread_yield(), 0;
}

} // namespace Sortix

namespace Sortix {
namespace Scheduler {

void ScheduleTrueThread()
{
	bool wasenabled = Interrupt::SetEnabled(false);
	if ( true_current_thread != current_thread )
	{
		current_thread->yield_to_tid = 0;
		first_runnable_thread = true_current_thread;
		kthread_yield();
	}
	Interrupt::SetEnabled(wasenabled);
}

void Init()
{
	first_runnable_thread = NULL;
	true_current_thread = NULL;
	idle_thread = NULL;
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
