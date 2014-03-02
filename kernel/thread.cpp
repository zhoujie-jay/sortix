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

    thread.cpp
    Describes a thread belonging to a process.

*******************************************************************************/

#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <sortix/exit.h>
#include <sortix/mman.h>
#include <sortix/signal.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>

namespace Sortix {

Thread::Thread()
{
	id = 0; // TODO: Make a thread id.
	process = NULL;
	prevsibling = NULL;
	nextsibling = NULL;
	scheduler_list_prev = NULL;
	scheduler_list_next = NULL;
	state = NONE;
	memset(&registers, 0, sizeof(registers));
	fsbase = 0;
	gsbase = 0;
	kernelstackpos = 0;
	kernelstacksize = 0;
	kernelstackmalloced = false;
	pledged_destruction = false;
	fpuinitialized = false;
	// If malloc isn't 16-byte aligned, then we can't rely on offsets in
	// our own class, so we'll just fix ourselves nicely up.
	unsigned long fpuaddr = ((unsigned long) fpuenv+16UL) & ~(16UL-1UL);
	fpuenvaligned = (uint8_t*) fpuaddr;
	sigemptyset(&signal_pending);
	sigemptyset(&signal_mask);
	memset(&signal_stack, 0, sizeof(signal_stack));
	signal_stack.ss_flags = SS_DISABLE;
}

Thread::~Thread()
{
	if ( process )
		process->OnThreadDestruction(this);
	assert(CurrentThread() != this);
	if ( kernelstackmalloced )
		delete[] (uint8_t*) kernelstackpos;
}

void Thread::SaveRegisters(const CPU::InterruptRegisters* src)
{
#if defined(__i386__)
	registers.eip = src->eip;
	registers.esp = src->esp;
	registers.eax = src->eax;
	registers.ebx = src->ebx;
	registers.ecx = src->ecx;
	registers.edx = src->edx;
	registers.edi = src->edi;
	registers.esi = src->esi;
	registers.ebp = src->ebp;
	registers.cs = src->cs;
	registers.ds = src->ds;
	registers.ss = src->ss;
	registers.eflags = src->eflags;
	registers.kerrno = src->kerrno;
	registers.signal_pending = src->signal_pending;
#elif defined(__x86_64__)
	registers.rip = src->rip;
	registers.rsp = src->rsp;
	registers.rax = src->rax;
	registers.rbx = src->rbx;
	registers.rcx = src->rcx;
	registers.rdx = src->rdx;
	registers.rdi = src->rdi;
	registers.rsi = src->rsi;
	registers.rbp = src->rbp;
	registers.r8 = src->r8;
	registers.r9 = src->r9;
	registers.r10 = src->r10;
	registers.r11 = src->r11;
	registers.r12 = src->r12;
	registers.r13 = src->r13;
	registers.r14 = src->r14;
	registers.r15 = src->r15;
	registers.cs = src->cs;
	registers.ds = src->ds;
	registers.ss = src->ss;
	registers.rflags = src->rflags;
	registers.kerrno = src->kerrno;
	registers.signal_pending = src->signal_pending;
#else
#warning "You need to add register saving support"
#endif
}

void Thread::LoadRegisters(CPU::InterruptRegisters* dest)
{
#if defined(__i386__)
	dest->eip = registers.eip;
	dest->esp = registers.esp;
	dest->eax = registers.eax;
	dest->ebx = registers.ebx;
	dest->ecx = registers.ecx;
	dest->edx = registers.edx;
	dest->edi = registers.edi;
	dest->esi = registers.esi;
	dest->ebp = registers.ebp;
	dest->cs = registers.cs;
	dest->ds = registers.ds;
	dest->ss = registers.ss;
	dest->eflags = registers.eflags;
	dest->kerrno = registers.kerrno;
	dest->signal_pending = registers.signal_pending;
#elif defined(__x86_64__)
	dest->rip = registers.rip;
	dest->rsp = registers.rsp;
	dest->rax = registers.rax;
	dest->rbx = registers.rbx;
	dest->rcx = registers.rcx;
	dest->rdx = registers.rdx;
	dest->rdi = registers.rdi;
	dest->rsi = registers.rsi;
	dest->rbp = registers.rbp;
	dest->r8 = registers.r8;
	dest->r9 = registers.r9;
	dest->r10 = registers.r10;
	dest->r11 = registers.r11;
	dest->r12 = registers.r12;
	dest->r13 = registers.r13;
	dest->r14 = registers.r14;
	dest->r15 = registers.r15;
	dest->cs = registers.cs;
	dest->ds = registers.ds;
	dest->ss = registers.ss;
	dest->rflags = registers.rflags;
	dest->kerrno = registers.kerrno;
	dest->signal_pending = registers.signal_pending;
#else
#warning "You need to add register loading support"
#endif
}

addr_t Thread::SwitchAddressSpace(addr_t newaddrspace)
{
	bool wasenabled = Interrupt::SetEnabled(false);
	addr_t result = addrspace;
	addrspace = newaddrspace;
	Memory::SwitchAddressSpace(newaddrspace);
	Interrupt::SetEnabled(wasenabled);
	return result;
}

Thread* CreateKernelThread(Process* process, CPU::InterruptRegisters* regs,
                           unsigned long fsbase, unsigned long gsbase)
{
#if defined(__x86_64__)
	if ( fsbase >> 48 != 0x0000 && fsbase >> 48 != 0xFFFF )
		return errno = EINVAL, (Thread*) NULL;
	if ( gsbase >> 48 != 0x0000 && gsbase >> 48 != 0xFFFF )
		return errno = EINVAL, (Thread*) NULL;
#endif

	assert(process && regs && process->addrspace);
	Thread* thread = new Thread;
	if ( !thread )
		return NULL;

	thread->addrspace = process->addrspace;
	thread->SaveRegisters(regs);
	thread->fsbase = fsbase;
	thread->gsbase = gsbase;

	kthread_mutex_lock(&process->threadlock);

	// Create the family tree.
	thread->process = process;
	Thread* firsty = process->firstthread;
	if ( firsty )
		firsty->prevsibling = thread;
	thread->nextsibling = firsty;
	process->firstthread = thread;

	kthread_mutex_unlock(&process->threadlock);

	return thread;
}

static void SetupKernelThreadRegs(CPU::InterruptRegisters* regs,
                                  void (*entry)(void*),
                                  void* user,
                                  uintptr_t stack,
                                  size_t stack_size)
{
#if defined(__i386__)
	uintptr_t* stack_values = (uintptr_t*) (stack + stack_size);

	assert(!((uintptr_t) stack_values & 3UL));
	assert(4 * sizeof(uintptr_t) <= stack_size);

	stack_values[-1] = (uintptr_t) 0; /* null eip */
	stack_values[-2] = (uintptr_t) 0; /* null ebp */
	stack_values[-3] = (uintptr_t) user; /* thread parameter */
	stack_values[-4] = (uintptr_t) kthread_exit; /* return to kthread_exit */

	regs->eip = (uintptr_t) entry;
	regs->esp = (uintptr_t) (stack_values - 4);
	regs->eax = 0;
	regs->ebx = 0;
	regs->ecx = 0;
	regs->edx = 0;
	regs->edi = 0;
	regs->esi = 0;
	regs->ebp = (uintptr_t) (stack_values - 2);
	regs->cs = KCS | KRPL;
	regs->ds = KDS | KRPL;
	regs->ss = KDS | KRPL;
	regs->eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	regs->kerrno = 0;
	regs->signal_pending = 0;
#elif defined(__x86_64__)
	if ( (stack & 15UL) == 8 && 8 <= stack_size )
	{
		stack += 8;
		stack_size -= 8;
	}

	uintptr_t* stack_values = (uintptr_t*) (stack + stack_size);

	assert(!((uintptr_t) stack_values & 15UL));
	assert(3 * sizeof(uintptr_t) <= stack_size);

	stack_values[-1] = (uintptr_t) 0; /* null rip */
	stack_values[-2] = (uintptr_t) 0; /* null rbp */
	stack_values[-3] = (uintptr_t) kthread_exit; /* return to kthread_exit */

	regs->rip = (uintptr_t) entry;
	regs->rsp = (uintptr_t) (stack_values - 3);
	regs->rax = 0;
	regs->rbx = 0;
	regs->rcx = 0;
	regs->rdx = 0;
	regs->rdi = (uintptr_t) user;
	regs->rsi = 0;
	regs->rbp = 0;
	regs->r8  = 0;
	regs->r9  = 0;
	regs->r10 = 0;
	regs->r11 = 0;
	regs->r12 = 0;
	regs->r13 = 0;
	regs->r14 = 0;
	regs->r15 = 0;
	regs->cs = KCS | KRPL;
	regs->ds = KDS | KRPL;
	regs->ss = KDS | KRPL;
	regs->rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	regs->kerrno = 0;
	regs->signal_pending = 0;
#else
#warning "You need to add thread register initialization support"
#endif
}

Thread* CreateKernelThread(Process* process, void (*entry)(void*), void* user,
                           size_t stacksize)
{
	const size_t DEFAULT_KERNEL_STACK_SIZE = 8 * 1024UL;
	if ( !stacksize )
		stacksize = DEFAULT_KERNEL_STACK_SIZE;
	uint8_t* stack = new uint8_t[stacksize];
	if ( !stack )
		return NULL;

	CPU::InterruptRegisters regs;
	SetupKernelThreadRegs(&regs, entry, user, (uintptr_t) stack, stacksize);

	Thread* thread = CreateKernelThread(process, &regs, 0, 0);
	if ( !thread ) { delete[] stack; return NULL; }

	thread->kernelstackpos = (uintptr_t) stack;
	thread->kernelstacksize = stacksize;
	thread->kernelstackmalloced = true;

	return thread;
}

Thread* CreateKernelThread(void (*entry)(void*), void* user, size_t stacksize)
{
	return CreateKernelThread(CurrentProcess(), entry, user, stacksize);
}

void StartKernelThread(Thread* thread)
{
	Scheduler::SetThreadState(thread, ThreadState::RUNNABLE);
}

Thread* RunKernelThread(Process* process, CPU::InterruptRegisters* regs)
{
	Thread* thread = CreateKernelThread(process, regs, 0, 0);
	if ( !thread )
		return NULL;
	StartKernelThread(thread);
	return thread;
}

Thread* RunKernelThread(Process* process, void (*entry)(void*), void* user,
                        size_t stacksize)
{
	Thread* thread = CreateKernelThread(process, entry, user, stacksize);
	if ( !thread )
		return NULL;
	StartKernelThread(thread);
	return thread;
}

Thread* RunKernelThread(void (*entry)(void*), void* user, size_t stacksize)
{
	Thread* thread = CreateKernelThread(entry, user, stacksize);
	if ( !thread )
		return NULL;
	StartKernelThread(thread);
	return thread;
}

static int sys_exit_thread(int requested_exit_code,
                           int flags,
                           const struct exit_thread* user_extended)
{
	if ( flags & ~(EXIT_THREAD_ONLY_IF_OTHERS |
	               EXIT_THREAD_UNMAP |
	               EXIT_THREAD_ZERO |
	               EXIT_THREAD_TLS_UNMAP |
	               EXIT_THREAD_PROCESS |
	               EXIT_THREAD_DUMP_CORE) )
		return errno = EINVAL, -1;

	if ( (flags & EXIT_THREAD_ONLY_IF_OTHERS) && (flags & EXIT_THREAD_PROCESS) )
		return errno = EINVAL, -1;

	Thread* thread = CurrentThread();
	Process* process = CurrentProcess();

	struct exit_thread extended;
	if ( !user_extended )
		memset(&extended, 0, sizeof(extended));
	else if ( !CopyFromUser(&extended, user_extended, sizeof(extended)) )
		return -1;

	extended.unmap_size = Page::AlignUp(extended.unmap_size);

	kthread_mutex_lock(&thread->process->threadlock);
	bool is_others = false;
	for ( Thread* iter = thread->process->firstthread;
	      !is_others && iter;
	     iter = iter->nextsibling )
	{
		if ( iter == thread )
			continue;
		if ( iter->pledged_destruction )
			continue;
		is_others = true;
	}
	if ( !(flags & EXIT_THREAD_ONLY_IF_OTHERS) || is_others )
		thread->pledged_destruction = true;
	bool are_threads_exiting = false;
	if ( (flags & EXIT_THREAD_PROCESS) || !is_others )
		process->threads_exiting = true;
	else if ( process->threads_exiting )
		are_threads_exiting = true;
	kthread_mutex_unlock(&thread->process->threadlock);

	// Self-destruct if another thread began exiting the process.
	if ( are_threads_exiting )
		kthread_exit();

	if ( (flags & EXIT_THREAD_ONLY_IF_OTHERS) && !is_others )
		return errno = ESRCH, -1;

	if ( flags & EXIT_THREAD_UNMAP &&
	     Page::IsAligned((uintptr_t) extended.unmap_from) &&
	     extended.unmap_size )
	{
		ScopedLock lock(&process->segment_lock);
		Memory::UnmapMemory(process, (uintptr_t) extended.unmap_from,
		                                         extended.unmap_size);
		Memory::Flush();
		// TODO: The segment is not actually removed!
	}

	if ( flags & EXIT_THREAD_TLS_UNMAP &&
	     Page::IsAligned((uintptr_t) extended.tls_unmap_from) &&
	     extended.tls_unmap_size )
	{
		ScopedLock lock(&process->segment_lock);
		Memory::UnmapMemory(process, (uintptr_t) extended.tls_unmap_from,
		                                         extended.tls_unmap_size);
		Memory::Flush();
	}

	if ( flags & EXIT_THREAD_ZERO )
		ZeroUser(extended.zero_from, extended.zero_size);

	if ( !is_others )
	{
		// Validate the requested exit code such that the process can't exit
		// with an impossible exit status or that it wasn't actually terminated.

		int the_nature = WNATURE(requested_exit_code);
		int the_status = WEXITSTATUS(requested_exit_code);
		int the_signal = WTERMSIG(requested_exit_code);

		if ( the_nature == WNATURE_EXITED )
			the_signal = 0;
		else if ( the_nature == WNATURE_SIGNALED )
		{
			if ( the_signal == 0 /* null signal */ ||
			     the_signal == SIGSTOP ||
			     the_signal == SIGTSTP ||
			     the_signal == SIGTTIN ||
			     the_signal == SIGTTOU ||
			     the_signal == SIGCONT )
				the_signal = SIGKILL;
			the_status = 128 + the_signal;
		}
		else
		{
			the_nature = WNATURE_SIGNALED;
			the_signal = SIGKILL;
		}

		requested_exit_code = WCONSTRUCT(the_nature, the_status, the_signal);

		thread->process->ExitWithCode(requested_exit_code);
	}

	kthread_exit();
}

void Thread::Init()
{
	Syscall::Register(SYSCALL_EXIT_THREAD, (void*) sys_exit_thread);
}

} // namespace Sortix
