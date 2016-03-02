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
 * x86-family/idt.h
 * Initializes and handles the IDT.
 */

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
