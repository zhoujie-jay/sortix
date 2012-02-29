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

	x86-family/gdt.h
	Initializes and handles the GDT and TSS.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_GDT_H
#define SORTIX_X86_FAMILY_GDT_H

namespace Sortix
{
	namespace GDT
	{
		// This structure contains the value of one GDT entry.
		// We use the attribute 'packed' to tell GCC not to change
		// any of the alignment in the structure.
		struct gdt_entry_struct
		{
			uint16_t limit_low;           // The lower 16 bits of the limit.
			uint16_t base_low;            // The lower 16 bits of the base.
			uint8_t  base_middle;         // The next 8 bits of the base.
			uint8_t  access;              // Access flags, determine what ring this segment can be used in.
			uint8_t  granularity;
			uint8_t  base_high;           // The last 8 bits of the base.
		} __attribute__((packed));

		struct gdt_entry64_struct
		{
			uint16_t limit_low;
			uint16_t base_low;
			uint8_t  base_middle;
			uint8_t access;
			uint8_t granularity;
			uint8_t base_high;
			uint32_t base_highest;
			uint32_t zero1;
		} __attribute__((packed));

		typedef struct gdt_entry_struct gdt_entry_t;
		typedef struct gdt_entry64_struct gdt_entry64_t;

		// This struct describes a GDT pointer. It points to the start of
		// our array of GDT entries, and is in the format required by the
		// lgdt instruction.
		struct gdt_ptr_struct
		{
			uint16_t limit;               // The upper 16 bits of all selector limits.
			addr_t base;                  // The address of the first gdt_entry_t struct.
		} __attribute__((packed));

		typedef struct gdt_ptr_struct gdt_ptr_t;

		// A struct describing a Task State Segment.
#ifdef PLATFORM_X86
		struct tss_entry_struct
		{
			uint32_t prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
			uint32_t esp0;       // The stack pointer to load when we change to kernel mode.
			uint32_t ss0;        // The stack segment to load when we change to kernel mode.
			uint32_t esp1;       // Unused...
			uint32_t ss1;
			uint32_t esp2;
			uint32_t ss2;
			uint32_t cr3;
			uint32_t eip;
			uint32_t eflags;
			uint32_t eax;
			uint32_t ecx;
			uint32_t edx;
			uint32_t ebx;
			uint32_t esp;
			uint32_t ebp;
			uint32_t esi;
			uint32_t edi;
			uint32_t es;         // The value to load into ES when we change to kernel mode.
			uint32_t cs;         // The value to load into CS when we change to kernel mode.
			uint32_t ss;         // The value to load into SS when we change to kernel mode.
			uint32_t ds;         // The value to load into DS when we change to kernel mode.
			uint32_t fs;         // The value to load into FS when we change to kernel mode.
			uint32_t gs;         // The value to load into GS when we change to kernel mode.
			uint32_t ldt;        // Unused...
			uint16_t trap;
			uint16_t iomap_base;
		} __attribute__((packed));
#else
		struct tss_entry_struct
		{
			uint32_t reserved1;
			uint64_t stack0;
			uint64_t stack1;
			uint64_t stack2;
			uint64_t reserved2;
			uint64_t ist[7];
			uint64_t reserved3;
			uint16_t reserved4;
			uint16_t iomap_base;
		} __attribute__((packed));
#endif

		typedef struct tss_entry_struct tss_entry_t; 

		void Init();
		void SetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
		void WriteTSS(int32_t num, uint16_t ss0, addr_t stack0);
		void SetKernelStack(size_t* stack);
	}
}

#endif

