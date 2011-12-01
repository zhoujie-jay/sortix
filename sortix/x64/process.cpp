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

	x64/process.cpp
	CPU-specific process code.

******************************************************************************/

#include "platform.h"
#include "process.h"

namespace Sortix
{
	void Process::ExecuteCPU(int argc, char** argv, addr_t stackpos, addr_t entry, CPU::InterruptRegisters* regs)
	{
		regs->rdi = argc;
		regs->rsi = (size_t) argv;
		regs->rip = entry;
		regs->userrsp = stackpos & ~(15UL);
		regs->rbp = stackpos;
	}
}
