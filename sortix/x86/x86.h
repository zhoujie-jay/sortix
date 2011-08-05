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

	x86
	CPU stuff for the x86 platform.

******************************************************************************/

#ifndef SORTIX_X86_H
#define SORTIX_X86_H

namespace Sortix
{
	namespace X86
	{
		void OutPortB(uint16_t Port, uint8_t Value);
		void OutPortW(uint16_t Port, uint16_t Value);
		void OutPortL(uint16_t Port, uint32_t Value);
		uint8_t InPortB(uint16_t Port);
		uint16_t InPortW(uint16_t Port);
		uint32_t InPortL(uint16_t Port);

		struct InterruptRegisters
		{
			uint32_t cr2;
			uint32_t ds; // Data segment selector
			uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
			uint32_t int_no, err_code; // Interrupt number and error code (if applicable)
			uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
		};
	}
}

#endif

