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

    x86-family/idt.h
    Initializes and handles the IDT.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_IDT_H
#define SORTIX_X86_FAMILY_IDT_H

namespace Sortix
{
	namespace IDT
	{
		// A struct describing an interrupt gate.
		struct idt_entry32_struct
		{
			uint16_t base_low;             // The lower 16 bits of the address to jump to when this interrupt fires.
			uint16_t sel;                 // Kernel segment selector.
			uint8_t  always0;             // This must always be zero.
			uint8_t  flags;               // More flags. See documentation.
			uint16_t base_high;             // The upper 16 bits of the address to jump to.
		} __attribute__((packed));

		struct idt_entry64_struct
		{
			uint16_t base_low;            // The lower 16 bits of the address to jump to when this interrupt fires.
			uint16_t sel;                 // Kernel segment selector.
			uint8_t  always0;             // This must always be zero.
			uint8_t  flags;               // More flags. See documentation.
			uint16_t base_high;           // The upper 16 bits of the address to jump to.
			uint32_t base_highest;
			uint32_t zero1;               // Reserved
		} __attribute__((packed));

		typedef struct idt_entry32_struct idt_entry32_t;
		typedef struct idt_entry64_struct idt_entry64_t;
#ifdef PLATFORM_X64
		typedef idt_entry64_t idt_entry_t;
#else
		typedef idt_entry32_t idt_entry_t;
#endif

		// A struct describing a pointer to an array of interrupt handlers.
		// This is in a format suitable for giving to 'lidt'.
		struct idt_ptr_struct
		{
			uint16_t limit;
			addr_t base;                // The address of the first element in our idt_entry_t array.
		} __attribute__((packed));

		typedef struct idt_ptr_struct idt_ptr_t;

		void Init();
		void SetGate(uint8_t num, addr_t base, uint16_t sel, uint8_t flags);
		void Flush();
	}
}

#endif
