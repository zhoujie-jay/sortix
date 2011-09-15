/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	thread.cpp
	x86 specific parts of thread.cpp.

******************************************************************************/

#include "platform.h"
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
	}

	const size_t FLAGS_CARRY        = (1<<0);
	const size_t FLAGS_RESERVED1    = (1<<1); /* read as one */
	const size_t FLAGS_PARITY       = (1<<2);
	const size_t FLAGS_RESERVED2    = (1<<3);
	const size_t FLAGS_AUX          = (1<<4);
	const size_t FLAGS_RESERVED3    = (1<<5);
	const size_t FLAGS_ZERO         = (1<<6);
	const size_t FLAGS_SIGN         = (1<<7);
	const size_t FLAGS_TRAP         = (1<<8);
	const size_t FLAGS_INTERRUPT    = (1<<9);
	const size_t FLAGS_DIRECTION    = (1<<10);
	const size_t FLAGS_OVERFLOW     = (1<<11);
	const size_t FLAGS_IOPRIVLEVEL  = (1<<12) | (1<<13);
	const size_t FLAGS_NESTEDTASK   = (1<<14);
	const size_t FLAGS_RESERVED4    = (1<<15);
	const size_t FLAGS_RESUME       = (1<<16);
	const size_t FLAGS_VIRTUAL8086  = (1<<17);
	const size_t FLAGS_ALIGNCHECK   = (1<<18);
	const size_t FLAGS_VIRTINTR     = (1<<19);
	const size_t FLAGS_VIRTINTRPEND = (1<<20);
	const size_t FLAGS_ID           = (1<<21);

	void CreateThreadCPU(Thread* thread, addr_t entry)
	{
		const uint32_t CS = 0x18;
		const uint32_t DS = 0x20;
		const uint32_t RPL = 0x3;

		CPU::InterruptRegisters regs;
		regs.eip = entry;
		regs.useresp = thread->stackpos + thread->stacksize;
		regs.eax = 0;
		regs.ebx = 0;
		regs.ecx = 0;
		regs.edx = 0;
		regs.edi = 0;
		regs.esi = 0;
		regs.ebp = thread->stackpos + thread->stacksize;
		regs.cs = CS | RPL;
		regs.ds = DS | RPL;
		regs.ss = DS | RPL;
		regs.eflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;

		thread->SaveRegisters(&regs);
	}

	void Thread::HandleSignalCPU(CPU::InterruptRegisters* regs)
	{
		regs->edi = currentsignal->signum;
		regs->eip = (size_t) sighandler;
	}
}
