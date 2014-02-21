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

addr_t Thread::SwitchAddressSpace(addr_t newaddrspace)
{
	bool wasenabled = Interrupt::SetEnabled(false);
	addr_t result = addrspace;
	addrspace = newaddrspace;
	Memory::SwitchAddressSpace(newaddrspace);
	Interrupt::SetEnabled(wasenabled);
	return result;
}

extern "C" void BootstrapKernelThread(void* user, ThreadEntry entry)
{
	entry(user);
	kthread_exit();
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

Thread* CreateKernelThread(Process* process, ThreadEntry entry, void* user,
                           size_t stacksize)
{
	const size_t DEFAULT_KERNEL_STACK_SIZE = 8 * 1024UL;
	if ( !stacksize )
		stacksize = DEFAULT_KERNEL_STACK_SIZE;
	uint8_t* stack = new uint8_t[stacksize];
	if ( !stack )
		return NULL;

	CPU::InterruptRegisters regs;
	SetupKernelThreadRegs(&regs, entry, user, (addr_t) stack, stacksize);

	Thread* thread = CreateKernelThread(process, &regs, 0, 0);
	if ( !thread ) { delete[] stack; return NULL; }

	thread->kernelstackpos = (addr_t) stack;
	thread->kernelstacksize = stacksize;
	thread->kernelstackmalloced = true;

	return thread;
}

Thread* CreateKernelThread(ThreadEntry entry, void* user, size_t stacksize)
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

Thread* RunKernelThread(Process* process, ThreadEntry entry, void* user,
                        size_t stacksize)
{
	Thread* thread = CreateKernelThread(process, entry, user, stacksize);
	if ( !thread )
		return NULL;
	StartKernelThread(thread);
	return thread;
}

Thread* RunKernelThread(ThreadEntry entry, void* user, size_t stacksize)
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
