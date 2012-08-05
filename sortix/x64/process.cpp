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

	x64/process.cpp
	CPU-specific process code.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/fork.h>
#include <libmaxsi/memory.h>
#include "process.h"

namespace Sortix
{
	void Process::ExecuteCPU(int argc, char** argv, int envc, char** envp,
	                         addr_t stackpos, addr_t entry,
	                         CPU::InterruptRegisters* regs)
	{
		const uint64_t CS = 0x18;
		const uint64_t DS = 0x20;
		const uint64_t RPL = 0x3;

		regs->rdi = argc;
		regs->rsi = (size_t) argv;
		regs->rdx = envc;
		regs->rcx = (size_t) envp;
		regs->rip = entry;
		regs->userrsp = stackpos & ~(15UL);
		regs->rbp = regs->userrsp;
		regs->cs = CS | RPL;
		regs->ds = DS | RPL;
		regs->ss = DS | RPL;
		regs->rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	}

	void InitializeThreadRegisters(CPU::InterruptRegisters* regs,
                                   const tforkregs_t* requested)
	{
		Maxsi::Memory::Set(regs, 0, sizeof(*regs));
		regs->rip = requested->rip;
		regs->userrsp = requested->rsp;
		regs->rax = requested->rax;
		regs->rbx = requested->rbx;
		regs->rcx = requested->rcx;
		regs->rdx = requested->rdx;
		regs->rdi = requested->rdi;
		regs->rsi = requested->rsi;
		regs->rbp = requested->rbp;
		regs->r8  = requested->r8;
		regs->r9  = requested->r9;
		regs->r10 = requested->r10;
		regs->r11 = requested->r11;
		regs->r12 = requested->r12;
		regs->r13 = requested->r13;
		regs->r14 = requested->r14;
		regs->r15 = requested->r15;
		regs->cs = 0x18 | 0x3;
		regs->ds = 0x20 | 0x3;
		regs->ss = 0x20 | 0x3;
		regs->rflags = FLAGS_RESERVED1 | FLAGS_INTERRUPT | FLAGS_ID;
	}
}
