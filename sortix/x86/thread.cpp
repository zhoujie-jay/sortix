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
	x86 specific parts of thread.cpp.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include "thread.h"

namespace Sortix
{
	void Thread::SaveRegisters(const CPU::InterruptRegisters* src)
	{
		registers.eip = src->eip;
		registers.useresp = src->useresp;
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
		dest->useresp = registers.useresp;
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
		regs->useresp = stack + stacksize;
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

	void Thread::HandleSignalFixupRegsCPU(CPU::InterruptRegisters* regs)
	{
		if ( regs->InUserspace() )
			return;
		uint32_t* params = (uint32_t*) regs->ebx;
		regs->eip = params[0];
		regs->eflags = params[2];
		regs->useresp = params[3];
		regs->cs = UCS | URPL;
		regs->ds = UDS | URPL;
		regs->ss = UDS | URPL;
	}

	void Thread::HandleSignalCPU(CPU::InterruptRegisters* regs)
	{
		const size_t STACK_ALIGNMENT = 16UL;
		const size_t RED_ZONE_SIZE = 128UL;
		regs->useresp -= RED_ZONE_SIZE;
		regs->useresp &= ~(STACK_ALIGNMENT-1UL);
		regs->ebp = regs->useresp;
		regs->edi = currentsignal;
		regs->eip = (size_t) sighandler;
		regs->eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
		regs->kerrno = 0;
		regs->signal_pending = 0;
	}

	extern "C" void asm_call_Thread__OnSigKill(void);

	void Thread::GotoOnSigKill(CPU::InterruptRegisters* regs)
	{
		regs->eip = (unsigned long) asm_call_Thread__OnSigKill;
		regs->edi = (unsigned long) this;
		// TODO: HACK: The -256 is because if we are jumping to the safe stack
		// we currently are on, this may not be fully supported by interrupt.s
		// that is quite aware of this (but isn't perfect). If our destination
		// is further down the stack, then we are probably safe.
		regs->useresp = regs->ebp = kernelstackpos + kernelstacksize - 256;
		regs->eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
		regs->cs = KCS | KRPL;
		regs->ds = KDS | KRPL;
		regs->ss = KDS | KRPL;
		regs->kerrno = 0;
		regs->signal_pending = 0;
	}
}
