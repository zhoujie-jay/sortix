/*
 * Copyright (c) 2011, 2014 Jonas 'Sortie' Termansen.
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
 * x86-family/x86-family.cpp
 * CPU stuff for the x86 CPU family.
 */

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

} // namespace CPU
} // namespace Sortix

namespace Sortix {

uint64_t sys_rdmsr(uint32_t msrid)
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

uint64_t sys_wrmsr(uint32_t msrid, uint64_t value)
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

} // namespace Sortix
