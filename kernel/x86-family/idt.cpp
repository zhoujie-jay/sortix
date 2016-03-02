/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * x86-family/idt.cpp
 * Initializes and handles the interrupt descriptor table.
 */

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
