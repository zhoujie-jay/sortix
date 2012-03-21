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

	x64.h
	CPU stuff for the x64 platform.

******************************************************************************/

#ifndef SORTIX_X64_H
#define SORTIX_X64_H

#include "../x86-family/x86-family.h"

namespace Sortix
{
	namespace X64
	{
		struct InterruptRegisters
		{
			uint64_t cr2;
			uint64_t ds; // Data segment selector
			uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
			uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
			uint64_t int_no, err_code; // Interrupt number and error code (if applicable)
			uint64_t rip, cs, rflags, userrsp, ss; // Pushed by the processor automatically.

		public:
			void LogRegisters() const;
		};

		struct SyscallRegisters
		{
			uint64_t cr2; // For compabillity with above, may be removed soon.
			uint64_t ds;
			uint64_t di, si, bp, trash, b, d, c; union { uint64_t a; uint64_t result; };
			uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
			uint64_t int_no, err_code; // Also compabillity.
			uint64_t ip, cs, flags, sp, ss;
		};
	}
}

#endif

