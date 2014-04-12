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

    x86-family/gdt.cpp
    Initializes and handles the GDT and TSS.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

#include <sortix/kernel/cpu.h>

#include "gdt.h"

namespace Sortix {
namespace GDT {

struct gdt_entry
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
};

struct gdt_entry64
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
	uint32_t base_highest;
	uint32_t reserved0;
} __attribute__((packed));

struct gdt_ptr
{
	uint16_t limit;
#if defined(__i386__)
	uint32_t base;
#else
	uint64_t base;
#endif
} __attribute__((packed));

#if defined(__i386__)
struct tss_entry
{
	uint32_t prev_tss; // The previous TSS - if we used hardware task switching this would form a linked list.
	uint32_t esp0; // The stack pointer to load when we change to kernel mode.
	uint32_t ss0; // The stack segment to load when we change to kernel mode.
	uint32_t esp1; // Unused...
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
	uint32_t es; // The value to load into ES when we change to kernel mode.
	uint32_t cs; // The value to load into CS when we change to kernel mode.
	uint32_t ss; // The value to load into SS when we change to kernel mode.
	uint32_t ds; // The value to load into DS when we change to kernel mode.
	uint32_t fs; // The value to load into FS when we change to kernel mode.
	uint32_t gs; // The value to load into GS when we change to kernel mode.
	uint32_t ldt; // Unused...
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));
#elif defined(__x86_64__)
struct tss_entry
{
	uint32_t reserved0;
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

const size_t GDT_NUM_ENTRIES = 7;
static struct gdt_entry gdt_entries[GDT_NUM_ENTRIES];

static struct tss_entry tss_entry;

const uint8_t GRAN_64_BIT_MODE = 1 << 5;
const uint8_t GRAN_32_BIT_MODE = 1 << 6;
const uint8_t GRAN_4KIB_BLOCKS = 1 << 7;

void SetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	struct gdt_entry* entry = (struct gdt_entry*) &gdt_entries[num];

	entry->base_low = base >> 0 & 0xFFFF;
	entry->base_middle = base >> 16 & 0xFF;
	entry->base_high = base >> 24 & 0xFF;

	entry->limit_low = limit & 0xFFFF;
	entry->granularity = (limit >> 16 & 0x0F) | (gran & 0xF0);

	entry->access = access;
}

void SetGate64(int32_t num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	struct gdt_entry64* entry = (struct gdt_entry64*) &gdt_entries[num];

	entry->base_low = base >> 0 & 0xFFFF;
	entry->base_middle = base >> 16 & 0xFF;
	entry->base_high = base >> 24 & 0xFF;
	entry->base_highest = base >> 32;

	entry->limit_low = limit & 0xFFFF;
	entry->granularity = (limit >> 16 & 0x0F) | (gran & 0xF0);

	entry->access = access;
	entry->reserved0 = 0;
}

void Init()
{

#if defined(__i386__)
	const uint8_t gran = GRAN_4KIB_BLOCKS | GRAN_32_BIT_MODE;
#elif defined(__x86_64__)
	const uint8_t gran = GRAN_4KIB_BLOCKS | GRAN_64_BIT_MODE;
#endif

	SetGate(0, 0, 0, 0, 0); // Null segment
	SetGate(1, 0, 0xFFFFFFFF, 0x9A, gran); // Code segment
	SetGate(2, 0, 0xFFFFFFFF, 0x92, gran); // Data segment
	SetGate(3, 0, 0xFFFFFFFF, 0xFA, gran); // User mode code segment
	SetGate(4, 0, 0xFFFFFFFF, 0xF2, gran); // User mode data segment

	WriteTSS(5, 0x10, 0x0);

	// Reload the Global Descriptor Table.
	volatile struct gdt_ptr gdt_ptr;
	gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_NUM_ENTRIES) - 1;
	gdt_ptr.base = (uintptr_t) &gdt_entries;
	asm volatile ("lgdt (%0)" : : "r"(&gdt_ptr));

	// Switch the current data segment.
	asm volatile ("mov %0, %%ds\n"
	              "mov %0, %%es\n"
	              "mov %0, %%fs\n"
	              "mov %0, %%gs\n"
	              "mov %0, %%ss\n" : :
	              "r"(KDS));

	// Switch the current code segment.
	#if defined(__i386__)
	asm volatile ("push %0\n"
	              "push $1f\n"
	              "retf\n"
	              "1:\n" : :
	              "r"(KCS));
	#elif defined(__x86_64__)
	asm volatile ("push %0\n"
	              "push $1f\n"
	              "retfq\n"
	              "1:\n" : :
	              "r"(KCS));
	#endif

	// Load the task state register - The index is 0x28, as it is the 5th
	// selector and each is 8 bytes long, but we set the bottom two bits (making
	// 0x2B) so that it has an RPL of 3, not zero.
	asm volatile ("ltr %%ax" : : "a"(0x2B));
}

// Initialise our task state segment structure.
void WriteTSS(int32_t num, uint16_t ss0, uintptr_t stack0)
{
	// First, let's compute the base and limit of our entry in the GDT.
	uintptr_t base = (uintptr_t) &tss_entry;
	uint32_t limit = sizeof(tss_entry) - 1;

	// Now, add our TSS descriptor's address to the GDT.
#if defined(__i386__)
	SetGate(num, base, limit, 0xE9, 0x00);
#elif defined(__x86_64__)
	SetGate64(num, base, limit, 0xE9, 0x00);
#endif

	// Ensure the descriptor is initially zero.
	memset(&tss_entry, 0, sizeof(tss_entry));

#if defined(__i386__)
	tss_entry.ss0  = ss0; // Set the kernel stack segment.
	tss_entry.esp0 = stack0; // Set the kernel stack pointer.

	// Here we set the cs, ss, ds, es, fs and gs entries in the TSS.
	// These specify what segments should be loaded when the processor
	// switches to kernel mode. Therefore they are just our normal
	// kernel code/data segments - 0x08 and 0x10 respectively, but with
	// the last two bits set, making 0x0b and 0x13. The setting of these
	// bits sets the RPL (requested privilege level) to 3, meaning that
	// this TSS can be used to switch to kernel mode from ring 3.
	tss_entry.cs = KCS | 0x3;
	tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = KDS | 0x3;
#elif defined(__x86_64__)
	(void) ss0;
	tss_entry.stack0 = stack0;
#endif
}

void SetKernelStack(uintptr_t stacklower, size_t stacksize, uintptr_t stackhigher)
{
#if defined(__i386__)
	(void) stacklower;
	(void) stacksize;
	tss_entry.esp0 = (uint32_t) stackhigher;
#elif defined(__x86_64__)
	(void) stacklower;
	(void) stacksize;
	tss_entry.stack0 = (uint64_t) stackhigher;
#endif
}

} // namespace GDT
} // namespace Sortix
