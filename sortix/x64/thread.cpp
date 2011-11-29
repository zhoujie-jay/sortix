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
	x64 specific parts of thread.cpp.

******************************************************************************/

#include "platform.h"
#include "thread.h"

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
		const uint64_t CS = 0x18;
		const uint64_t DS = 0x20;
		const uint64_t RPL = 0x3;

		CPU::InterruptRegisters regs;
		regs.rip = entry;
		regs.userrsp = thread->stackpos + thread->stacksize;
		regs.rax = 0;
		regs.rbx = 0;
		regs.rcx = 0;
		regs.rdx = 0;
		regs.rdi = 0;
		regs.rsi = 0;
		regs.rbp = thread->stackpos + thread->stacksize;
		regs.r8 = 0;
		regs.r9 = 0;
		regs.r10 = 0;
		regs.r11 = 0;
		regs.r12 = 0;
		regs.r13 = 0;
		regs.r14 = 0;
		regs.r15 = 0;
		regs.cs = CS | RPL;
		regs.ds = DS | RPL;
		regs.ss = DS | RPL;
		regs.rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;

		thread->SaveRegisters(&regs);
	}

	void Thread::HandleSignalCPU(CPU::InterruptRegisters* regs)
	{
		regs->rdi = currentsignal->signum;
		regs->rip = (size_t) sighandler;
	}
}
