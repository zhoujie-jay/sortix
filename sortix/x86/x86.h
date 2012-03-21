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

	x86.h
	CPU stuff for the x86 platform.

******************************************************************************/

#ifndef SORTIX_X86_H
#define SORTIX_X86_H

#include "../x86-family/x86-family.h"

namespace Sortix
{
	namespace X86
	{
		struct InterruptRegisters
		{
			uint32_t cr2;
			uint32_t ds; // Data segment selector
			uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
			uint32_t int_no, err_code; // Interrupt number and error code (if applicable)
			uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.

		public:
			void LogRegisters() const;

		};

		struct SyscallRegisters
		{
			uint32_t cr2; // For compabillity with above, may be removed soon.
			uint32_t ds;
			uint32_t di, si, bp, trash, b, d, c; union { uint32_t a; uint32_t result; };
			uint32_t int_no, err_code; // Also compabillity.
			uint32_t ip, cs, flags, sp, ss;
		};
	}
}

#endif

