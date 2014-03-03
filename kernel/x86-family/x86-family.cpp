/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    x86-family/x86-family.cpp
    CPU stuff for the x86 CPU family.

*******************************************************************************/

#include <errno.h>
#include <msr.h>
#include <stdint.h>

#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/syscall.h>

#include "gdt.h"

namespace Sortix {
namespace CPU {

void Reboot()
{
	// Keyboard interface IO port: data and control.
	const uint16_t KEYBOARD_INTERFACE = 0x64;

	// Keyboard IO port.
	const uint16_t KEYBOARD_IO = 0x60;

	// Keyboard data is in buffer (output buffer is empty) (bit 0).
	const uint8_t KEYBOARD_DATA = 1 << 0;

	// User data is in buffer (command buffer is empty) (bit 1).
	const uint8_t USER_DATA = 1 << 1;

	// Disable interrupts.
	asm volatile("cli");

	// Clear all keyboard buffers (output and command buffers).
	uint8_t byte;
	do
	{
		byte = inport8(KEYBOARD_INTERFACE);
		if ( byte & KEYBOARD_DATA )
			inport8(KEYBOARD_IO);
	} while ( byte & USER_DATA );

	// CPU reset command.
	uint8_t KEYBOARD_RESET_CPU = 0xFE;

	// Now pulse the CPU reset line and reset.
	outport8(KEYBOARD_INTERFACE, KEYBOARD_RESET_CPU);

	// If that didn't work, just halt.
	asm volatile("hlt");
}

void ShutDown()
{
	// TODO: Unimplemented, just reboot.
	Reboot();
}

static uint64_t sys_rdmsr(uint32_t msrid)
{
	switch ( msrid )
	{
	case MSRID_FSBASE:
#if defined(__i386__)
		return GDT::GetFSBase();
#elif defined(__x86_64__)
		return rdmsr(msrid);
#endif
	case MSRID_GSBASE:
#if defined(__i386__)
		return GDT::GetGSBase();
#elif defined(__x86_64__)
		return rdmsr(msrid);
#endif
	default:
		return errno = EPERM, UINT64_MAX;
	};
}

static uint64_t sys_wrmsr(uint32_t msrid, uint64_t value)
{
	switch ( msrid )
	{
	case MSRID_FSBASE:
#if defined(__i386__)
		if ( UINT32_MAX < value )
			return errno = EINVAL, UINT64_MAX;
		return GDT::SetFSBase((uint32_t) value), value;
#elif defined(__x86_64__)
		if ( value >> 48 != 0x0000 && value >> 48 != 0xFFFF )
			return errno = EINVAL, UINT64_MAX;
		return wrmsr(msrid, value);
#endif
	case MSRID_GSBASE:
#if defined(__i386__)
		if ( UINT32_MAX < value )
			return errno = EINVAL, UINT64_MAX;
		return GDT::SetGSBase((uint32_t) value), value;
#elif defined(__x86_64__)
		if ( value >> 48 != 0x0000 && value >> 48 != 0xFFFF )
			return errno = EINVAL, UINT64_MAX;
		return wrmsr(msrid, value);
#endif
	default:
		return errno = EPERM, UINT64_MAX;
	};
}

void Init()
{
	Syscall::Register(SYSCALL_RDMSR, (void*) sys_rdmsr);
	Syscall::Register(SYSCALL_WRMSR, (void*) sys_wrmsr);
}

} // namespace CPU
} // namespace Sortix
