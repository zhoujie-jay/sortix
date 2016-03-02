/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sortix/kernel/scheduler.h
 * Decides the order to execute threads in and switching between them.
 */

#ifndef INCLUDE_SORTIX_KERNEL_SCHEDULER_H
#define INCLUDE_SORTIX_KERNEL_SCHEDULER_H

#include <sortix/kernel/decl.h>
#include <sortix/kernel/registers.h>

namespace Sortix {
class Process;
class Thread;
} // namespace Sortix

namespace Sortix {
enum ThreadState { NONE, RUNNABLE, BLOCKING, DEAD };
} // namespace Sortix

namespace Sortix {
namespace Scheduler {

#if defined(__i386__) || defined(__x86_64__)
static inline void Yield()
{
	asm volatile ("int $129");
}
__attribute__ ((noreturn))
static inline void ExitThread()
{
	asm volatile ("int $132");
	__builtin_unreachable();
}
#endif

void Switch(struct interrupt_context* intctx);
void SetThreadState(Thread* thread, ThreadState state);
ThreadState GetThreadState(Thread* thread);
void SetIdleThread(Thread* thread);
void SetInitProcess(Process* init);
Process* GetInitProcess();
Process* GetKernelProcess();
void InterruptYieldCPU(struct interrupt_context* intctx, void* user);
void ThreadExitCPU(struct interrupt_context* intctx, void* user);
void SaveInterruptedContext(const struct interrupt_context* intctx,
                            struct thread_registers* registers);
void LoadInterruptedContext(struct interrupt_context* intctx,
                            const struct thread_registers* registers);
void ScheduleTrueThread();

} // namespace Scheduler
} // namespace Sortix

#endif
