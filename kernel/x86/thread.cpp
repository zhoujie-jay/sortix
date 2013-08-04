/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    x86/thread.cpp
    x86 specific parts of thread.cpp.

*******************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/thread.h>

namespace Sortix {

void Thread::SaveRegisters(const CPU::InterruptRegisters* src)
{
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
}

void Thread::LoadRegisters(CPU::InterruptRegisters* dest)
{
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
}

extern "C" void asm_call_BootstrapKernelThread(void);

void SetupKernelThreadRegs(CPU::InterruptRegisters* regs, ThreadEntry entry,
                           void* user, addr_t stack, size_t stacksize)
{
	// Instead of directly calling the desired entry point, we call a small
	// stub that calls it for us and then destroys the kernel thread if
	// the entry function returns. Note that since we use a stack based
	// calling convention, we go through a proxy that uses %edi and %esi
	// as parameters and pushes them to the stack and then does the call.
	regs->eip = (addr_t) asm_call_BootstrapKernelThread;
	regs->esp = stack + stacksize - sizeof(size_t);
	*((size_t*) regs->esp) = 0; /* back tracing stops at NULL rip */
	regs->eax = 0;
	regs->ebx = 0;
	regs->ecx = 0;
	regs->edx = 0;
	regs->edi = (addr_t) user;
	regs->esi = (addr_t) entry;
	regs->ebp = 0;
	regs->cs = KCS | KRPL;
	regs->ds = KDS | KRPL;
	regs->ss = KDS | KRPL;
	regs->eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	regs->kerrno = 0;
	regs->signal_pending = 0;
}

} // namespace Sortix
