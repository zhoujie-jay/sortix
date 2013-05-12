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

    x64/thread.cpp
    x64 specific parts of thread.cpp.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/thread.h>

namespace Sortix
{
	void Thread::SaveRegisters(const CPU::InterruptRegisters* src)
	{
		registers.rip = src->rip;
		registers.userrsp = src->userrsp;
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
	}

	void Thread::LoadRegisters(CPU::InterruptRegisters* dest)
	{
		dest->rip = registers.rip;
		dest->userrsp = registers.userrsp;
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
	}

	void SetupKernelThreadRegs(CPU::InterruptRegisters* regs, ThreadEntry entry,
	                           void* user, addr_t stack, size_t stacksize)
	{
		// Instead of directly calling the desired entry point, we call a small
		// stub that calls it for us and then destroys the kernel thread if
		// the entry function returns. Note that since we use a register based
		// calling convention, we call BootstrapKernelThread directly.
		regs->rip = (addr_t) BootstrapKernelThread;
		regs->userrsp = stack + stacksize - sizeof(size_t);
		*((size_t*) regs->userrsp) = 0; /* back tracing stops at NULL rip */
		regs->rax = 0;
		regs->rbx = 0;
		regs->rcx = 0;
		regs->rdx = 0;
		regs->rdi = (addr_t) user;
		regs->rsi = (addr_t) entry;
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
	}

	void Thread::HandleSignalFixupRegsCPU(CPU::InterruptRegisters* regs)
	{
		if ( regs->InUserspace() )
			return;
		regs->rip = regs->rdi;
		regs->rflags = regs->rsi;
		regs->userrsp = regs->r8;
		regs->cs = UCS | URPL;
		regs->ds = UDS | URPL;
		regs->ss = UDS | URPL;
	}

	void Thread::HandleSignalCPU(CPU::InterruptRegisters* regs)
	{
		const size_t STACK_ALIGNMENT = 16UL;
		const size_t RED_ZONE_SIZE = 128UL;
		regs->userrsp -= RED_ZONE_SIZE;
		regs->userrsp &= ~(STACK_ALIGNMENT-1UL);
		regs->rbp = regs->userrsp;
		regs->rdi = currentsignal;
		regs->rip = (size_t) sighandler;
		regs->rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
		regs->kerrno = 0;
		regs->signal_pending = 0;
	}

	void Thread::GotoOnSigKill(CPU::InterruptRegisters* regs)
	{
		regs->rip = (unsigned long) Thread__OnSigKill;
		regs->rdi = (unsigned long) this;
		regs->userrsp = regs->rbp = kernelstackpos + kernelstacksize;
		regs->rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
		regs->cs = KCS | KRPL;
		regs->ds = KDS | KRPL;
		regs->ss = KDS | KRPL;
		regs->kerrno = 0;
		regs->signal_pending = 0;
	}
}
