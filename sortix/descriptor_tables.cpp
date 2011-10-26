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

	descriptor_tables.cpp
	Initializes and handles the GDT, TSS and IDT.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "descriptor_tables.h"
#include "panic.h"
#include "syscall.h"

using namespace Maxsi;

namespace Sortix
{
	namespace GDT
	{
		extern "C" void gdt_flush(addr_t);
		extern "C" void tss_flush();
	
		const size_t GDT_NUM_ENTRIES = 7;
		gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
		gdt_ptr_t   gdt_ptr;
		tss_entry_t tss_entry;

		const uint8_t GRAN_64_BIT_MODE = 1<<5;
		const uint8_t GRAN_32_BIT_MODE = 1<<6;
		const uint8_t GRAN_4KIB_BLOCKS = 1<<7;
		
		void Init()
		{
			gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_NUM_ENTRIES) -  1;
			gdt_ptr.base  = (addr_t) &gdt_entries;

#ifdef PLATFORM_X86
			const uint8_t gran = GRAN_4KIB_BLOCKS | GRAN_32_BIT_MODE;
#elif defined(PLATFORM_X64)
			const uint8_t gran = GRAN_4KIB_BLOCKS | GRAN_64_BIT_MODE;
#endif

			SetGate(0, 0, 0, 0, 0);                // Null segment
			SetGate(1, 0, 0xFFFFFFFF, 0x9A, gran); // Code segment
			SetGate(2, 0, 0xFFFFFFFF, 0x92, gran); // Data segment
			SetGate(3, 0, 0xFFFFFFFF, 0xFA, gran); // User mode code segment
			SetGate(4, 0, 0xFFFFFFFF, 0xF2, gran); // User mode data segment

			WriteTSS(5, 0x10, 0x0);

			if ( gdt_ptr.base != (addr_t) &gdt_entries )
			{
				// If this happens, then either there is a bug in the GDT entry
				// writing code - or the struct gdt_entries above is too small.
				Panic("Whoops, someone overwrote the GDT pointer while writing "
				      "to the GDT entries!");
			}

			gdt_flush((addr_t) &gdt_ptr);
			tss_flush();
		}

		// Set the value of a GDT entry.
		void SetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
		{
			gdt_entry_t* entry  = (gdt_entry_t*) (&gdt_entries[num]);

			entry->base_low      = (base & 0xFFFF);
			entry->base_middle   = (base >> 16) & 0xFF;
			entry->base_high     = (base >> 24) & 0xFF;

			entry->limit_low     = (limit & 0xFFFF);
			entry->granularity   = ((limit >> 16) & 0x0F) | (gran & 0xF0);

			entry->access        = access;
		}

		// Set the value of a GDT entry.
		void SetGate64(int32_t num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran)
		{
			gdt_entry64_t* entry = (gdt_entry64_t*) (&gdt_entries[num]);

			entry->base_low      = (base & 0xFFFF);
			entry->base_middle   = (base >> 16) & 0xFF;
			entry->base_high     = (base >> 24) & 0xFF;
			entry->base_highest  = (base >> 32);

			entry->limit_low     = (limit & 0xFFFF);
			entry->granularity   = ((limit >> 16) & 0x0F) | (gran & 0xF0);

			entry->access        = access;
			entry->zero1         = 0;
		}

		// Initialise our task state segment structure.
		void WriteTSS(int32_t num, uint16_t ss0, addr_t stack0)
		{
			// First, let's compute the base and limit of our entry in the GDT.
			addr_t base = (addr_t) &tss_entry;
			uint32_t limit = base + sizeof(tss_entry);

			// Now, add our TSS descriptor's address to the GDT.
#ifdef PLATFORM_X86
			SetGate(num, base, limit, 0xE9, 0x00);
#elif defined(PLATFORM_X64)
			SetGate64(num, base, limit, 0xE9, 0x00);
#endif

			// Ensure the descriptor is initially zero.
			Memory::Set(&tss_entry, 0, sizeof(tss_entry));

#ifdef PLATFORM_X86
			tss_entry.ss0  = ss0;  // Set the kernel stack segment.
			tss_entry.esp0 = stack0; // Set the kernel stack pointer.

			// Here we set the cs, ss, ds, es, fs and gs entries in the TSS.
			// These specify what segments should be loaded when the processor
			// switches to kernel mode. Therefore they are just our normal
			// kernel code/data segments - 0x08 and 0x10 respectively, but with
			// the last two bits set, making 0x0b and 0x13. The setting of these
			// bits sets the RPL (requested privilege level) to 3, meaning that
			// this TSS can be used to switch to kernel mode from ring 3.
			tss_entry.cs = 0x08 | 0x3;
			tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x10 | 0x3;
#elif defined(PLATFORM_X64)
			tss_entry.stack0 = stack0;
#endif
		}

		void SetKernelStack(size_t* stack)
		{
#ifdef PLATFORM_X86
			tss_entry.esp0 = (uint32_t) stack;
#elif defined(PLATFORM_X64)
			tss_entry.stack0 = (uint64_t) stack;
#else
			#warning "TSS is not yet supported on this arch!"
			while(true);
#endif
		}
	}

	namespace IDT
	{
		// Lets us access our ASM functions from our C++ code.
		extern "C" void idt_flush(uint32_t);

		idt_entry_t idt_entries[256];
		idt_ptr_t   idt_ptr;

		void Init()
		{
#ifdef PLATFORM_X86
			idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
			idt_ptr.base  = (uint32_t)&idt_entries;

			Memory::Set(&idt_entries, 0, sizeof(idt_entry_t)*256);

			// Remap the irq table.
			X86::OutPortB(0x20, 0x11);
			X86::OutPortB(0xA0, 0x11);
			X86::OutPortB(0x21, 0x20);
			X86::OutPortB(0xA1, 0x28);
			X86::OutPortB(0x21, 0x04);
			X86::OutPortB(0xA1, 0x02);
			X86::OutPortB(0x21, 0x01);
			X86::OutPortB(0xA1, 0x01);
			X86::OutPortB(0x21, 0x0);
			X86::OutPortB(0xA1, 0x0);

			SetGate( 0, (uint32_t) isr0 , 0x08, 0x8E);
			SetGate( 1, (uint32_t) isr1 , 0x08, 0x8E);
			SetGate( 2, (uint32_t) isr2 , 0x08, 0x8E);
			SetGate( 3, (uint32_t) isr3 , 0x08, 0x8E);
			SetGate( 4, (uint32_t) isr4 , 0x08, 0x8E);
			SetGate( 5, (uint32_t) isr5 , 0x08, 0x8E);
			SetGate( 6, (uint32_t) isr6 , 0x08, 0x8E);
			SetGate( 7, (uint32_t) isr7 , 0x08, 0x8E);
			SetGate( 8, (uint32_t) isr8 , 0x08, 0x8E);
			SetGate( 9, (uint32_t) isr9 , 0x08, 0x8E);
			SetGate(10, (uint32_t) isr10, 0x08, 0x8E);
			SetGate(11, (uint32_t) isr11, 0x08, 0x8E);
			SetGate(12, (uint32_t) isr12, 0x08, 0x8E);
			SetGate(13, (uint32_t) isr13, 0x08, 0x8E);
			SetGate(14, (uint32_t) isr14, 0x08, 0x8E);
			SetGate(15, (uint32_t) isr15, 0x08, 0x8E);
			SetGate(16, (uint32_t) isr16, 0x08, 0x8E);
			SetGate(17, (uint32_t) isr17, 0x08, 0x8E);
			SetGate(18, (uint32_t) isr18, 0x08, 0x8E);
			SetGate(19, (uint32_t) isr19, 0x08, 0x8E);
			SetGate(20, (uint32_t) isr20, 0x08, 0x8E);
			SetGate(21, (uint32_t) isr21, 0x08, 0x8E);
			SetGate(22, (uint32_t) isr22, 0x08, 0x8E);
			SetGate(23, (uint32_t) isr23, 0x08, 0x8E);
			SetGate(24, (uint32_t) isr24, 0x08, 0x8E);
			SetGate(25, (uint32_t) isr25, 0x08, 0x8E);
			SetGate(26, (uint32_t) isr26, 0x08, 0x8E);
			SetGate(27, (uint32_t) isr27, 0x08, 0x8E);
			SetGate(28, (uint32_t) isr28, 0x08, 0x8E);
			SetGate(29, (uint32_t) isr29, 0x08, 0x8E);
			SetGate(30, (uint32_t) isr30, 0x08, 0x8E);
			SetGate(31, (uint32_t) isr31, 0x08, 0x8E);
			SetGate(32, (uint32_t) irq0, 0x08, 0x8E);
			SetGate(33, (uint32_t) irq1, 0x08, 0x8E);
			SetGate(34, (uint32_t) irq2, 0x08, 0x8E);
			SetGate(35, (uint32_t) irq3, 0x08, 0x8E);
			SetGate(36, (uint32_t) irq4, 0x08, 0x8E);
			SetGate(37, (uint32_t) irq5, 0x08, 0x8E);
			SetGate(38, (uint32_t) irq6, 0x08, 0x8E);
			SetGate(39, (uint32_t) irq7, 0x08, 0x8E);
			SetGate(40, (uint32_t) irq8, 0x08, 0x8E);
			SetGate(41, (uint32_t) irq9, 0x08, 0x8E);
			SetGate(42, (uint32_t) irq10, 0x08, 0x8E);
			SetGate(43, (uint32_t) irq11, 0x08, 0x8E);
			SetGate(44, (uint32_t) irq12, 0x08, 0x8E);
			SetGate(45, (uint32_t) irq13, 0x08, 0x8E);
			SetGate(46, (uint32_t) irq14, 0x08, 0x8E);
			SetGate(47, (uint32_t) irq15, 0x08, 0x8E);
			SetGate(128, (uint32_t) syscall_handler, 0x08, 0x8E | 0x60); // System Calls

			idt_flush((uint32_t)&idt_ptr);
#else
			#warning "IDT is not yet supported on this arch!"
			while(true);
#endif
		}

		void SetGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
		{
			idt_entries[num].base_lo = base & 0xFFFF;
			idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

			idt_entries[num].sel     = sel;
			idt_entries[num].always0 = 0;
			idt_entries[num].flags   = flags;
		}
	}
}

