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

    thread.cpp
    Describes a thread belonging to a process.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/mman.h>
#include <sortix/signal.h>

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
	schedulerlistprev = NULL;
	schedulerlistnext = NULL;
	state = NONE;
	memset(&registers, 0, sizeof(registers));
	fsbase = 0;
	gsbase = 0;
	kernelstackpos = 0;
	kernelstacksize = 0;
	kernelstackmalloced = false;
	currentsignal = 0;
	siglevel = 0;
	sighandler = NULL;
	terminated = false;
	fpuinitialized = false;
	// If malloc isn't 16-byte aligned, then we can't rely on offsets in
	// our own class, so we'll just fix ourselves nicely up.
	unsigned long fpuaddr = ((unsigned long) fpuenv+16UL) & ~(16UL-1UL);
	fpuenvaligned = (uint8_t*) fpuaddr;
}

Thread::~Thread()
{
	if ( process )
		process->OnThreadDestruction(this);
	assert(CurrentThread() != this);
	if ( kernelstackmalloced )
		delete[] (uint8_t*) kernelstackpos;
	terminated = true;
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

// Last chance to clean up user-space things before this thread dies.
void Thread::LastPrayer()
{
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

void Thread::HandleSignal(CPU::InterruptRegisters* regs)
{
	int signum = signalqueue.Pop(currentsignal);
	regs->signal_pending = 0;

	if ( !signum )
		return;

	if ( !sighandler )
		return;

	if ( SIG_NUM_LEVELS <= siglevel )
		return;

	// Signals can't return to kernel mode because the kernel stack may have
	// been overwritten by a system call during the signal handler. Correct
	// the return state so it returns to userspace and not the kernel.
	if ( !regs->InUserspace() )
		HandleSignalFixupRegsCPU(regs);

	if ( signum == SIGKILL )
	{
		// We need to run the OnSigKill method here with interrupts enabled
		// and on our own stack. But this method this may have been called
		// from the scheduler on any stack, so we need to do a little
		// bootstrap and switch to our own stack.
		GotoOnSigKill(regs);
		return;
	}

	int level = siglevel++;
	signums[level] = currentsignal = signum;
	memcpy(sigregs + level, regs, sizeof(*regs));

	HandleSignalCPU(regs);
}

void Thread::HandleSigreturn(CPU::InterruptRegisters* regs)
{
	if ( !siglevel )
		return;

	siglevel--;

	currentsignal = siglevel ? signums[siglevel-1] : 0;
	memcpy(regs, sigregs + siglevel, sizeof(*regs));
	regs->signal_pending = 0;

	// Check if a more important signal is pending.
	HandleSignal(regs);
}

extern "C" void Thread__OnSigKill(Thread* thread)
{
	thread->OnSigKill();
}

void Thread::OnSigKill()
{
	LastPrayer();
	kthread_exit();
}

void Thread::SetHavePendingSignals()
{
	// TODO: This doesn't really work if Interrupt::IsCPUInterrupted()!
	if ( CurrentThread() == this )
		asm_signal_is_pending = 1;
	else
		registers.signal_pending = 1;
}

bool Thread::DeliverSignal(int signum)
{
	if ( signum <= 0 || 128 <= signum )
		return errno = EINVAL, false;

	bool wasenabled = Interrupt::SetEnabled(false);
	signalqueue.Push(signum);
	SetHavePendingSignals();
	Interrupt::SetEnabled(wasenabled);

	return true;
}

static int sys_kill(pid_t pid, int signum)
{
	// Protect the system idle process.
	if ( !pid )
		return errno = EPERM, -1;

	// TODO: Implement that pid == -1 means all processes!
	bool process_group = pid < 0 ? (pid = -pid, true) : false;

	// If we kill our own process, deliver the signal to this thread.
	Thread* currentthread = CurrentThread();
	if ( currentthread->process->pid == pid )
			return currentthread->DeliverSignal(signum) ? 0 : -1;

	// TODO: Race condition: The process could be deleted while we use it.
	Process* process = Process::Get(pid);
	if ( !process )
		return errno = ESRCH, -1;

	// TODO: Protect init?
	// TODO: Check for permission.
	// TODO: Check for zombies.

	return process_group ?
	       process->DeliverGroupSignal(signum) ? 0 : -1 :
	       process->DeliverSignal(signum) ? 0 : -1;
}

static int sys_raise(int signum)
{
	return CurrentThread()->DeliverSignal(signum) ? 0 : -1;
}

static int sys_register_signal_handler(sighandler_t sighandler)
{
	CurrentThread()->sighandler = sighandler;
	return 0;
}

void Thread::Init()
{
	Syscall::Register(SYSCALL_KILL, (void*) sys_kill);
	Syscall::Register(SYSCALL_RAISE, (void*) sys_raise);
	Syscall::Register(SYSCALL_REGISTER_SIGNAL_HANDLER, (void*) sys_register_signal_handler);
}

} // namespace Sortix
