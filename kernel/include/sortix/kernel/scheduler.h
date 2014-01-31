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

    sortix/kernel/scheduler.h
    Decides the order to execute threads in and switching between them.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_SCHEDULER_H
#define INCLUDE_SORTIX_KERNEL_SCHEDULER_H

#include <sortix/kernel/decl.h>

namespace Sortix {
class Process;
class Thread;
} // namespace Sortix

namespace Sortix {
namespace CPU {
struct InterruptRegisters;
} // namespace CPU
} // namespace Sortix

namespace Sortix {
enum ThreadState { NONE, RUNNABLE, BLOCKING, DEAD };
} // namespace Sortix

namespace Sortix {
namespace Scheduler {

void Init();
void Switch(CPU::InterruptRegisters* regs);
#if defined(__i386__) || defined(__x86_64__)
inline static void Yield() { asm volatile ("int $129"); }
__attribute__ ((noreturn))
inline static void ExitThread()
{
	asm volatile ("int $132");
	__builtin_unreachable();
}
#endif
void SetThreadState(Thread* thread, ThreadState state);
ThreadState GetThreadState(Thread* thread);
void SetIdleThread(Thread* thread);
void SetDummyThreadOwner(Process* process);
void SetInitProcess(Process* init);
Process* GetInitProcess();
Process* GetKernelProcess();
void InterruptYieldCPU(CPU::InterruptRegisters* regs, void* user);
void ThreadExitCPU(CPU::InterruptRegisters* regs, void* user);

} // namespace Scheduler
} // namespace Sortix

#endif
