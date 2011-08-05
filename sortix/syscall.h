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

#include "scheduler.h"

#ifdef PLATFORM_X86
	#define SYSPARAM __attribute__ ((aligned (4)));
#elif defined(PLATFORM_X64)
	#define SYSPARAM __attribute__ ((aligned (8)));
#endif

#define USING_OLD_SYSCALL

namespace Sortix
{
	namespace Syscall
	{
		void Init();
		void OnCall(CPU::InterruptRegisters* R);

#ifdef USING_OLD_SYSCALL
		typedef void (*Syscall)(CPU::InterruptRegisters* r);
#else
		typedef void (*Syscall)(Thread* R);
#endif
	}
}

#endif

