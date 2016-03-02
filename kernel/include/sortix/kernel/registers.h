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
 * sortix/kernel/registers.h
 * Register structures and platform-specific bits.
 */

#ifndef INCLUDE_SORTIX_KERNEL_REGISTERS_H
#define INCLUDE_SORTIX_KERNEL_REGISTERS_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {
// CPU flag register bits for 32-bit and 64-bit x86.
#if defined(__i386__) || defined(__x86_64__)
const size_t FLAGS_CARRY        = 1 << 0; // 0x000001
const size_t FLAGS_RESERVED1    = 1 << 1; // 0x000002, read as one
const size_t FLAGS_PARITY       = 1 << 2; // 0x000004
const size_t FLAGS_RESERVED2    = 1 << 3; // 0x000008
const size_t FLAGS_AUX          = 1 << 4; // 0x000010
const size_t FLAGS_RESERVED3    = 1 << 5; // 0x000020
const size_t FLAGS_ZERO         = 1 << 6; // 0x000040
const size_t FLAGS_SIGN         = 1 << 7; // 0x000080
const size_t FLAGS_TRAP         = 1 << 8; // 0x000100
const size_t FLAGS_INTERRUPT    = 1 << 9; // 0x000200
const size_t FLAGS_DIRECTION    = 1 << 10; // 0x000400
const size_t FLAGS_OVERFLOW     = 1 << 11; // 0x000800
const size_t FLAGS_IOPRIVLEVEL  = 1 << 12 | 1 << 13;
const size_t FLAGS_NESTEDTASK   = 1 << 14; // 0x004000
const size_t FLAGS_RESERVED4    = 1 << 15; // 0x008000
const size_t FLAGS_RESUME       = 1 << 16; // 0x010000
const size_t FLAGS_VIRTUAL8086  = 1 << 17; // 0x020000
const size_t FLAGS_ALIGNCHECK   = 1 << 18; // 0x040000
const size_t FLAGS_VIRTINTR     = 1 << 19; // 0x080000
const size_t FLAGS_VIRTINTRPEND = 1 << 20; // 0x100000
const size_t FLAGS_ID           = 1 << 21; // 0x200000
#endif

// i386 registers structures.
#if defined(__i386__)
const uint32_t KCS = 0x08;
const uint32_t KDS = 0x10;
const uint32_t KRPL = 0x0;
const uint32_t UCS = 0x18;
const uint32_t UDS = 0x20;
const uint32_t URPL = 0x3;
const uint32_t RPLMASK = 0x3;
#define GDT_FS_ENTRY 6
#define GDT_GS_ENTRY 7

struct interrupt_context
{
	uint32_t signal_pending;
	uint32_t kerrno;
	uint32_t cr2;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t not_esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t int_no;
	uint32_t err_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
};

__attribute__((unused))
static inline bool InUserspace(const struct interrupt_context* intctx)
{
	return (intctx->cs & RPLMASK) != KRPL;
}

struct thread_registers
{
	uint32_t signal_pending;
	uint32_t kerrno;
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t edi;
	uint32_t esi;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	uint32_t eflags;
	uint32_t fsbase;
	uint32_t gsbase;
	uint32_t cr3;
	uint32_t kernel_stack;
	uint32_t cs;
	uint32_t ds;
	uint32_t ss;
	__attribute__((aligned(16))) uint8_t fpuenv[512];
};
#endif

// x86_64 registers structures.
#if defined(__x86_64__)
const uint64_t KCS = 0x08;
const uint64_t KDS = 0x10;
const uint64_t KRPL = 0x0;
const uint64_t UCS = 0x18;
const uint64_t UDS = 0x20;
const uint64_t URPL = 0x3;
const uint64_t RPLMASK = 0x3;

struct interrupt_context
{
	uint64_t signal_pending;
	uint64_t kerrno;
	uint64_t cr2;
	uint64_t ds;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rbp;
	uint64_t not_rsp;
	uint64_t rbx;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rax;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t int_no;
	uint64_t err_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

__attribute__((unused))
static inline bool InUserspace(const struct interrupt_context* intctx)
{
	return (intctx->cs & RPLMASK) != KRPL;
}

struct thread_registers
{
	uint64_t signal_pending;
	uint64_t kerrno;
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rsp;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t rip;
	uint64_t rflags;
	uint64_t fsbase;
	uint64_t gsbase;
	uint64_t cr3;
	uint64_t kernel_stack;
	uint64_t cs;
	uint64_t ds;
	uint64_t ss;
	__attribute__((aligned(16))) uint8_t fpuenv[512];
};
#endif

void LogInterruptContext(const struct interrupt_context* intctx);
__attribute__((noreturn))
void LoadRegisters(const struct thread_registers* registers);

} // namespace Sortix

#endif
