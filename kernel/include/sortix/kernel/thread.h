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

    sortix/kernel/thread.h
    Describes a thread belonging to a process.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_THREAD_H
#define INCLUDE_SORTIX_KERNEL_THREAD_H

#include <sortix/sigaction.h>
#include <sortix/signal.h>
#include <sortix/sigset.h>
#include <sortix/stack.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/registers.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>

namespace Sortix {

class Process;
class Thread;

// These functions create a new kernel process but doesn't start it.
Thread* CreateKernelThread(Process* process, struct thread_registers* regs);
Thread* CreateKernelThread(Process* process, void (*entry)(void*), void* user,
                           size_t stacksize = 0);
Thread* CreateKernelThread(void (*entry)(void*), void* user, size_t stacksize = 0);

// This function can be used to start a thread from the above functions.
void StartKernelThread(Thread* thread);

// Alternatively, these functions both create and start the thread.
Thread* RunKernelThread(Process* process, struct thread_registers* regs);
Thread* RunKernelThread(Process* process, void (*entry)(void*), void* user,
                        size_t stacksize = 0);
Thread* RunKernelThread(void (*entry)(void*), void* user, size_t stacksize = 0);

class Thread
{
public:
	static void Init();

public:
	Thread();
	~Thread();

public:
	struct thread_registers registers;
	uint8_t* self_allocation;
	size_t id;
	Process* process;
	Thread* prevsibling;
	Thread* nextsibling;
	Thread* scheduler_list_prev;
	Thread* scheduler_list_next;
	volatile ThreadState state;
	sigset_t signal_pending;
	sigset_t signal_mask;
	stack_t signal_stack;
	addr_t kernelstackpos;
	size_t kernelstacksize;
	bool kernelstackmalloced;
	bool pledged_destruction;

public:
	void HandleSignal(struct interrupt_context* intctx);
	void HandleSigreturn(struct interrupt_context* intctx);
	bool DeliverSignal(int signum);
	bool DeliverSignalUnlocked(int signum);

};

Thread* AllocateThread();
void FreeThread(Thread* thread);

Thread* CurrentThread();

} // namespace Sortix

#endif
