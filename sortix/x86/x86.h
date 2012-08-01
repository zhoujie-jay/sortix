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

	x86.h
	CPU stuff for the x86 platform.

*******************************************************************************/

#ifndef SORTIX_X86_H
#define SORTIX_X86_H

#include "../x86-family/x86-family.h"

namespace Sortix
{
	namespace X86
	{
		struct InterruptRegisters
		{
			uint32_t signal_pending, kerrno, cr2;
			uint32_t ds; // Data segment selector
			uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
			uint32_t int_no, err_code; // Interrupt number and error code (if applicable)
			uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.

		public:
			void LogRegisters() const;
			bool InUserspace() const { return (cs & 0x3) != 0; }

		};
	}

	const uint64_t KCS = 0x08;
	const uint64_t KDS = 0x10;
	const uint64_t KRPL = 0x0;
	const uint64_t UCS = 0x18;
	const uint64_t UDS = 0x20;
	const uint64_t URPL = 0x3;
}

#endif

