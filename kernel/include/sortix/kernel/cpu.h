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

    sortix/kernel/cpu.h
    CPU-specific declarations.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_CPU_H
#define INCLUDE_SORTIX_KERNEL_CPU_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/decl.h>

namespace Sortix {

// Functions for 32-bit and 64-bit x86.
#if defined(__i386__) || defined(__x86_64__)
namespace CPU {
void Reboot();
void ShutDown();
} // namespace CPU
#endif

// CPU flag register bits for 32-bit and 64-bit x86.
#if defined(__i386__) || defined(__x86_64__)
const size_t FLAGS_CARRY        = 1<<0; // 0x000001
const size_t FLAGS_RESERVED1    = 1<<1; // 0x000002, read as one
const size_t FLAGS_PARITY       = 1<<2; // 0x000004
const size_t FLAGS_RESERVED2    = 1<<3; // 0x000008
const size_t FLAGS_AUX          = 1<<4; // 0x000010
const size_t FLAGS_RESERVED3    = 1<<5; // 0x000020
const size_t FLAGS_ZERO         = 1<<6; // 0x000040
const size_t FLAGS_SIGN         = 1<<7; // 0x000080
const size_t FLAGS_TRAP         = 1<<8; // 0x000100
const size_t FLAGS_INTERRUPT    = 1<<9; // 0x000200
const size_t FLAGS_DIRECTION    = 1<<10; // 0x000400
const size_t FLAGS_OVERFLOW     = 1<<11; // 0x000800
const size_t FLAGS_IOPRIVLEVEL  = 1<<12 | 1<<13;
const size_t FLAGS_NESTEDTASK   = 1<<14; // 0x004000
const size_t FLAGS_RESERVED4    = 1<<15; // 0x008000
const size_t FLAGS_RESUME       = 1<<16; // 0x010000
const size_t FLAGS_VIRTUAL8086  = 1<<17; // 0x020000
const size_t FLAGS_ALIGNCHECK   = 1<<18; // 0x040000
const size_t FLAGS_VIRTINTR     = 1<<19; // 0x080000
const size_t FLAGS_VIRTINTRPEND = 1<<20; // 0x100000
const size_t FLAGS_ID           = 1<<21; // 0x200000
#endif

// x86 interrupt registers structure.
#if defined(__i386__)
namespace X86 {
struct InterruptRegisters
{
	uint32_t signal_pending, kerrno, cr2;
	uint32_t ds; // Data segment selector
	uint32_t edi, esi, ebp, not_esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t int_no, err_code; // Interrupt number and error code (if applicable)
	uint32_t eip, cs, eflags, esp, ss; // Pushed by the processor automatically.

public:
	void LogRegisters() const;
	bool InUserspace() const { return (cs & 0x3) != 0; }

};
} // namespace X86
#endif

// x86_64 interrupt
#if defined(__x86_64__)
namespace X64 {
struct InterruptRegisters
{
	uint64_t signal_pending, kerrno, cr2;
	uint64_t ds; // Data segment selector
	uint64_t rdi, rsi, rbp, not_rsp, rbx, rdx, rcx, rax;
	uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
	uint64_t int_no, err_code; // Interrupt number and error code (if applicable)
	uint64_t rip, cs, rflags, rsp, ss; // Pushed by the processor automatically.

public:
	void LogRegisters() const;
	bool InUserspace() const { return (cs & 0x3) != 0; }

};
} // namespace X64
#endif

// Segment values for 32-bit and 64-bit x86.
#if defined(__i386__) || defined(__x86_64__)
const uint64_t KCS = 0x08;
const uint64_t KDS = 0x10;
const uint64_t KRPL = 0x0;
const uint64_t UCS = 0x18;
const uint64_t UDS = 0x20;
const uint64_t URPL = 0x3;
#endif

// Portable functions for loading registers.
namespace CPU {
extern "C" __attribute__((noreturn))
void load_registers(InterruptRegisters* regs, size_t size);

__attribute__((noreturn))
inline void LoadRegisters(InterruptRegisters* regs)
{
	load_registers(regs, sizeof(*regs));
}
} // namespace CPU

} // namespace Sortix

#endif
