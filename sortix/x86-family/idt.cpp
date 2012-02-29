/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	x86-family/idt.cpp
	Initializes and handles the IDT.

*******************************************************************************/

#include "../platform.h"
#include <libmaxsi/memory.h>
#include "idt.h"

using namespace Maxsi;

namespace Sortix
{
	namespace IDT
	{
		extern "C" void idt_flush(addr_t);

		idt_entry_t idt_entries[256];
		idt_ptr_t   idt_ptr;

		void Init()
		{
			idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
			idt_ptr.base  = (addr_t) &idt_entries;

			Memory::Set(&idt_entries, 0, sizeof(idt_entry_t)*256);
		}

		void Flush()
		{
			idt_flush((addr_t) &idt_ptr);
		}

		void SetGate(uint8_t num, addr_t base, uint16_t sel, uint8_t flags)
		{
			idt_entries[num].base_low = base & 0xFFFF;
			idt_entries[num].base_high = (base >> 16) & 0xFFFF;
#ifdef PLATFORM_X64
			idt_entries[num].base_highest = (base >> 32 ) & 0xFFFFFFFFU;
			idt_entries[num].zero1 = 0;
#endif

			idt_entries[num].sel     = sel;
			idt_entries[num].always0 = 0;
			idt_entries[num].flags   = flags;
		}
	}
}

