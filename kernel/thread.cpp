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
#include <stdlib.h>
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

void* operator new (size_t /*size*/, void* address) throw()
{
	return address;
}

namespace Sortix {

Thread* AllocateThread()
{
	uint8_t* allocation = (uint8_t*) malloc(sizeof(class Thread) + 16);
	if ( !allocation )
		return NULL;

	uint8_t* aligned = allocation;
	if ( ((uintptr_t) aligned & 0xFUL) )
		aligned = (uint8_t*) (((uintptr_t) aligned + 16) & ~0xFUL);

	assert(!((uintptr_t) aligned & 0xFUL));
	Thread* thread = new (aligned) Thread;
	assert(!((uintptr_t) thread->registers.fpuenv & 0xFUL));
	return thread->self_allocation = allocation, thread;
}

void FreeThread(Thread* thread)
{
	uint8_t* allocation = thread->self_allocation;
	thread->~Thread();
	free(allocation);
}

Thread::Thread()
{
	assert(!((uintptr_t) registers.fpuenv & 0xFUL));
	id = 0; // TODO: Make a thread id.
	process = NULL;
	prevsibling = NULL;
	nextsibling = NULL;
	scheduler_list_prev = NULL;
	scheduler_list_next = NULL;
	state = NONE;
	memset(&registers, 0, sizeof(registers));
	kernelstackpos = 0;
	kernelstacksize = 0;
	kernelstackmalloced = false;
	pledged_destruction = false;
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

Thread* CreateKernelThread(Process* process, struct thread_registers* regs)
{
	assert(process && regs && process->addrspace);

#if defined(__x86_64__)
	if ( regs->fsbase >> 48 != 0x0000 && regs->fsbase >> 48 != 0xFFFF )
		return errno = EINVAL, (Thread*) NULL;
	if ( regs->gsbase >> 48 != 0x0000 && regs->gsbase >> 48 != 0xFFFF )
		return errno = EINVAL, (Thread*) NULL;
#endif

	Thread* thread = AllocateThread();
	if ( !thread )
		return NULL;

	memcpy(&thread->registers, regs, sizeof(struct thread_registers));

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

static void SetupKernelThreadRegs(struct thread_registers* regs,
                                  Process* process,
                                  void (*entry)(void*),
                                  void* user,
                                  uintptr_t stack,
                                  size_t stack_size)
{
	memset(regs, 0, sizeof(*regs));

	size_t stack_alignment = 16;
	while ( stack & (stack_alignment-1) )
	{
		assert(stack_size);
		stack++;
		stack_size--;
	}

	stack_size &= ~(stack_alignment-1);

#if defined(__i386__)
	uintptr_t* stack_values = (uintptr_t*) (stack + stack_size);

	assert(5 * sizeof(uintptr_t) <= stack_size);

	/* -- 16-byte aligned -- */
	/* -1 padding */
	stack_values[-2] = (uintptr_t) 0; /* null eip */
	stack_values[-3] = (uintptr_t) 0; /* null ebp */
	stack_values[-4] = (uintptr_t) user; /* thread parameter */
	/* -- 16-byte aligned -- */
	stack_values[-5] = (uintptr_t) kthread_exit; /* return to kthread_exit */
	/* upcoming ebp */
	/* -7 padding */
	/* -8 padding */
	/* -- 16-byte aligned -- */

	regs->eip = (uintptr_t) entry;
	regs->esp = (uintptr_t) (stack_values - 5);
	regs->eax = 0;
	regs->ebx = 0;
	regs->ecx = 0;
	regs->edx = 0;
	regs->edi = 0;
	regs->esi = 0;
	regs->ebp = (uintptr_t) (stack_values - 3);
	regs->cs = KCS | KRPL;
	regs->ds = KDS | KRPL;
	regs->ss = KDS | KRPL;
	regs->eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	regs->kerrno = 0;
	regs->signal_pending = 0;
	regs->kernel_stack = stack + stack_size;
	regs->cr3 = process->addrspace;
#elif defined(__x86_64__)
	uintptr_t* stack_values = (uintptr_t*) (stack + stack_size);

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
	regs->kernel_stack = stack + stack_size;
	regs->cr3 = process->addrspace;
#else
#warning "You need to add kernel thread register initialization support"
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

	struct thread_registers regs;
	SetupKernelThreadRegs(&regs, process, entry, user, (uintptr_t) stack, stacksize);

	Thread* thread = CreateKernelThread(process, &regs);
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

Thread* RunKernelThread(Process* process, struct thread_registers* regs)
{
	Thread* thread = CreateKernelThread(process, regs);
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
