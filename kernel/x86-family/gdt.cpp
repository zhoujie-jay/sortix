/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * x86-family/gdt.cpp
 * GDT and TSS.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/registers.h>

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

#if defined(__x86_64__)
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
};
#endif

#if defined(__i386__)
struct tss_entry
{
	uint32_t prev_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
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
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
};
#elif defined(__x86_64__)
struct tss_entry
{
	uint32_t reserved0;
	uint64_t stack0; /* This is not naturally aligned, so packed is needed. */
	uint64_t stack1;
	uint64_t stack2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iomap_base;
} __attribute__((packed));
#endif

extern "C" {

const size_t STACK_SIZE = 64*1024;
extern size_t stack[STACK_SIZE / sizeof(size_t)];

struct tss_entry tss =
{
#if defined(__i386__)
	.prev_tss = 0,                                                     /* c++ */
	.esp0 = 0 /*(uintptr_t) stack + sizeof(stack)*/,
	.ss0 = 0x10 /* Kernel Data Segment */,
	.esp1 = 0,                                                         /* c++ */
	.ss1 = 0,                                                          /* c++ */
	.esp2 = 0,                                                         /* c++ */
	.ss2 = 0,                                                          /* c++ */
	.cr3 = 0,                                                          /* c++ */
	.eip = 0,                                                          /* c++ */
	.eflags = 0,                                                       /* c++ */
	.eax = 0,                                                          /* c++ */
	.ecx = 0,                                                          /* c++ */
	.edx = 0,                                                          /* c++ */
	.ebx = 0,                                                          /* c++ */
	.esp = 0,                                                          /* c++ */
	.ebp = 0,                                                          /* c++ */
	.esi = 0,                                                          /* c++ */
	.edi = 0,                                                          /* c++ */
	.es = 0x13 /* Kernel Data Segment */,
	.cs = 0x0B /* Kernel Code Segment */,
	.ss = 0,                                                           /* c++ */
	.ds = 0x13 /* Kernel Data Segment */,
	.fs = 0x13 /* Kernel Data Segment */,
	.gs = 0x13 /* Kernel Data Segment */,
	.ldt = 0,                                                          /* c++ */
	.trap = 0,                                                         /* c++ */
	.iomap_base = 0,                                                   /* c++ */
#elif defined(__x86_64__)
	.reserved0 = 0,                                                    /* c++ */
	.stack0 = 0 /*(uintptr_t) stack + sizeof(stack)*/,
	.stack1 = 0,                                                       /* c++ */
	.stack2 = 0,                                                       /* c++ */
	.reserved1 = 0,                                                    /* c++ */
	.ist = { 0, 0, 0, 0, 0, 0, 0},
	.reserved2 = 0,
	.reserved3 = 0,
	.iomap_base = 0,
#endif
};

} /* extern "C" */

#define GRAN_64_BIT_MODE (1 << 5)
#define GRAN_32_BIT_MODE (1 << 6)
#define GRAN_4KIB_BLOCKS (1 << 7)

#define GDT_ENTRY(base, limit, access, granularity) \
	{ (limit) & 0xFFFF,                                /* limit_low */ \
	  (uint16_t) ((base) >> 0 & 0xFFFF),               /* base_low */ \
	  (uint8_t) ((base) >> 16 & 0xFF),                 /* base_middle */ \
	  (access) & 0xFF,                                 /* access */ \
	  ((limit) >> 16 & 0x0F) | ((granularity) & 0xF0), /* granularity */ \
	  (uint8_t) ((base) >> 24 & 0xFF),                 /* base_high */ }

#if defined(__x86_64__)
#define GDT_ENTRY64(base, limit, access, granularity) \
	{ (limit) & 0xFFFF,                                /* limit_low */ \
	  (uint16_t) ((base) >> 0 & 0xFFFF),               /* base_low */ \
	  (uint8_t) ((base) >> 16 & 0xFF),                 /* base_middle */ \
	  (access) & 0xFF,                                 /* access */ \
	  ((limit) >> 16 & 0x0F) | ((granularity) & 0xF0), /* granularity */ \
	  (uint8_t) ((base) >> 24 & 0xFF),                 /* base_high */ }, \
	{ (uint16_t) ((base) >> 32 & 0xFFFF),              /* base_highest */ \
	  (uint16_t) ((base) >> 48 & 0xFFFF),              /* base_highest */ \
	  0,                                               /* reserved0 */ \
	  0,                                               /* reserved0 */ \
	  0,                                               /* reserved0 */ \
	  0,                                               /* reserved0 */ }
#endif

extern "C" {

struct gdt_entry gdt[] =
{
	/* 0x00: Null segment */
	GDT_ENTRY(0, 0, 0, 0),

#if defined(__i386__)
	/* 0x08: Kernel Code Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0x9A, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x10: Kernel Data Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0x92, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x18: User Code Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xFA, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x20: User Data Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xF2, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x28: Task Switch Segment. */
	GDT_ENTRY(0 /*((uintptr_t) &tss)*/, sizeof(tss) - 1, 0xE9, 0x00),

	/* 0x30: F Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xF2, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x38: G Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xF2, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS),
#elif defined(__x86_64__)
	/* 0x08: Kernel Code Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0x9A, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x10: Kernel Data Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0x92, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x18: User Code Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xFA, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x20: User Data Segment. */
	GDT_ENTRY(0, 0xFFFFFFFF, 0xF2, GRAN_64_BIT_MODE | GRAN_4KIB_BLOCKS),

	/* 0x28: Task Switch Segment. */
	GDT_ENTRY64((uint64_t) 0 /*((uintptr_t) &tss)*/, sizeof(tss) - 1, 0xE9, 0x00),
#endif
};

uint16_t gdt_size_minus_one = sizeof(gdt) - 1;

} /* extern "C" */

uintptr_t GetKernelStack()
{
#if defined(__i386__)
	return tss.esp0;
#elif defined(__x86_64__)
	return tss.stack0;
#endif
}

void SetKernelStack(uintptr_t stack_pointer)
{
	assert((stack_pointer & 0xF) == 0);
#if defined(__i386__)
	tss.esp0 = (uint32_t) stack_pointer;
#elif defined(__x86_64__)
	tss.stack0 = (uint64_t) stack_pointer;
#endif
}

#if defined(__i386__)
uint32_t GetFSBase()
{
	struct gdt_entry* entry = gdt + GDT_FS_ENTRY;
	return (uint32_t) entry->base_low << 0 |
	       (uint32_t) entry->base_middle << 16 |
	       (uint32_t) entry->base_high << 24;
}

uint32_t GetGSBase()
{
	struct gdt_entry* entry = gdt + GDT_GS_ENTRY;
	return (uint32_t) entry->base_low << 0 |
	       (uint32_t) entry->base_middle << 16 |
	       (uint32_t) entry->base_high << 24;
}

void SetFSBase(uint32_t fsbase)
{
	struct gdt_entry* entry = gdt + GDT_FS_ENTRY;
	entry->base_low = fsbase >> 0 & 0xFFFF;
	entry->base_middle = fsbase >> 16 & 0xFF;
	entry->base_high = fsbase >> 24 & 0xFF;
	asm volatile ("mov %0, %%fs" : : "r"(GDT_FS_ENTRY << 3 | URPL));
}

void SetGSBase(uint32_t gsbase)
{
	struct gdt_entry* entry = gdt + GDT_GS_ENTRY;
	entry->base_low = gsbase >> 0 & 0xFFFF;
	entry->base_middle = gsbase >> 16 & 0xFF;
	entry->base_high = gsbase >> 24 & 0xFF;
	asm volatile ("mov %0, %%gs" : : "r"(GDT_GS_ENTRY << 3 | URPL));
}
#endif

} // namespace GDT
} // namespace Sortix
