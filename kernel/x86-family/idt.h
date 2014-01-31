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

    x86-family/idt.h
    Initializes and handles the IDT.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_IDT_H
#define SORTIX_X86_FAMILY_IDT_H

#include <stdint.h>

namespace Sortix {
namespace IDT {

struct idt_entry
{
	uint16_t handler_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t handler_high;
#if defined(__x86_64__)
	uint32_t handler_highest;
	uint32_t reserved1;
#endif
};

static const uint8_t FLAG_PRESENT = 1 << 7;
static const uint8_t FLAG_DPL_SHIFT = 5;
static const uint8_t FLAG_DPL_BITS = 2;
static const uint8_t FLAG_TYPE_SHIFT = 0;
static const uint8_t FLAG_TYPE_BITS = 4;

static const uint8_t TYPE_INTERRUPT = 0xE;
static const uint8_t TYPE_TRAP = 0xF;

void Set(struct idt_entry* table, size_t length);
void SetEntry(struct idt_entry* entry, uintptr_t handler, uint16_t selector,
              uint8_t flags, uint8_t ist);

} // namespace IDT
} // namespace Sortix

#endif
