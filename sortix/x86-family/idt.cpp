/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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
    Initializes and handles the interrupt descriptor table.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "idt.h"

namespace Sortix {
namespace IDT {

struct idt_entry
{
	uint16_t handler_low;
	uint16_t sel;
	uint8_t reserved0;
	uint8_t flags;
	uint16_t handler_high;
#if defined(__x86_64__)
	uint32_t handler_highest;
	uint32_t reserved1;
#endif
};

struct idt_ptr
{
	uint16_t limit;
#if defined(__x86_64__)
	uint64_t idt_ptr;
#else
	uint32_t idt_ptr;
#endif
} __attribute__((packed));

static struct idt_entry idt_entries[256];

void Init()
{
	volatile struct idt_ptr ptr;
	ptr.limit = sizeof(idt_entries) - 1;
	ptr.idt_ptr = (unsigned long) &idt_entries;
	asm volatile ("lidt (%0)" : : "r"(&ptr));
	memset(&idt_entries, 0, sizeof(idt_entries));
}

void SetEntry(uint8_t num, uintptr_t handler, uint16_t sel, uint8_t flags)
{
	idt_entries[num].flags = flags;
	idt_entries[num].reserved0 = 0;
	idt_entries[num].sel = sel;
	idt_entries[num].handler_low = handler >> 0 & 0xFFFF;
	idt_entries[num].handler_high = handler >> 16 & 0xFFFF;
#if defined(__x86_64__)
	idt_entries[num].handler_highest = handler >> 32 & 0xFFFFFFFFU;
	idt_entries[num].reserved1 = 0;
#endif
}

} // namespace IDT
} // namespace Sortix
