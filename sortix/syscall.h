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

	syscall.h
	Handles system calls from userspace safely.

******************************************************************************/

#ifndef SORTIX_SYSCALL_H
#define SORTIX_SYSCALL_H

#include "syscallnum.h"

namespace Sortix
{
	namespace Syscall
	{
		void Init();
		void Register(size_t index, void* funcptr);

		// Aborts the current system call such that the current thread is marked
		// as blocking, and control is transferred to the next runnable thread.
		// This allows the thread to wait for an condition that makes the thread
		// runnable again and stores the return value in the proper register.
		// Once called, you may safely return from the system call in good faith
		// that its return value shall be discarded.
		void Incomplete();

		// For when you want the syscall exit code not to modify registers.
		void AsIs();

		CPU::InterruptRegisters* InterruptRegs();
		CPU::SyscallRegisters* SyscallRegs();
	}
}

extern "C" void syscall_handler();

#endif

