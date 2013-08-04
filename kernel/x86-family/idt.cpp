/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

void Set(struct idt_entry* table, size_t length)
{
	size_t limit = sizeof(idt_entry) * length - 1;
#if defined(__x86_64__)
	asm volatile ("subq $10, %%rsp\n\t"
	              "movw %w0, 0(%%rsp)\n\t"
	              "movq %1, 2(%%rsp)\n\t"
	              "lidt (%%rsp)\n\t"
	              "addq $10, %%rsp" : : "rN"(limit), "r"(table));
#else
	asm volatile ("subl $6, %%esp\n\t"
	              "movw %w0, 0(%%esp)\n\t"
	              "movl %1, 2(%%esp)\n\t"
	              "lidt (%%esp)\n\t"
	              "addl $6, %%esp" : : "rN"(limit), "r"(table));
#endif
}

void SetEntry(struct idt_entry* entry, uintptr_t handler, uint16_t selector, uint8_t flags, uint8_t ist)
{
	entry->flags = flags;
	entry->ist = ist;
	entry->selector = selector;
	entry->handler_low = handler >> 0 & 0xFFFF;
	entry->handler_high = handler >> 16 & 0xFFFF;
#if defined(__x86_64__)
	entry->handler_highest = handler >> 32 & 0xFFFFFFFFU;
	entry->reserved1 = 0;
#endif
}

} // namespace IDT
} // namespace Sortix
