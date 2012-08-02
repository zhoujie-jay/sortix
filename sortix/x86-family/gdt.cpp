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

	x86-family/gdt.cpp
	Initializes and handles the GDT and TSS.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/memory.h>
#include "gdt.h"

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

		void SetKernelStack(addr_t stacklower, size_t stacksize, addr_t stackhigher)
		{
#ifdef PLATFORM_X86
			tss_entry.esp0 = (uint32_t) stackhigher;
#elif defined(PLATFORM_X64)
			tss_entry.stack0 = (uint64_t) stackhigher;
#else
			#warning "TSS is not yet supported on this arch!"
			while(true);
#endif
		}
	}
}

