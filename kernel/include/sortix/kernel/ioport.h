/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/kernel/ioport.h
    IO ports.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_IOPORT_H
#define INCLUDE_SORTIX_KERNEL_IOPORT_H

#if !(defined(__i386__) || defined(__x86_64__))
#error "This hardware platform doesn't have IO ports"
#endif

#include <stdint.h>

namespace Sortix {

__attribute__((unused))
static inline uint8_t outport8(uint16_t port, uint8_t value)
{
	asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
	return value;
}

__attribute__((unused))
static inline uint16_t outport16(uint16_t port, uint16_t value)
{
	asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
	return value;
}

__attribute__((unused))
static inline uint32_t outport32(uint16_t port, uint32_t value)
{
	asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
	return value;
}

__attribute__((unused))
static inline uint8_t inport8(uint16_t port)
{
	uint8_t result;
	asm volatile("inb %1, %0" : "=a" (result) : "dN" (port));
	return result;
}

__attribute__((unused))
static inline uint16_t inport16(uint16_t port)
{
	uint16_t result;
	asm volatile("inw %1, %0" : "=a" (result) : "dN" (port));
	return result;
}

__attribute__((unused))
static inline uint32_t inport32(uint16_t port)
{
	uint32_t result;
	asm volatile("inl %1, %0" : "=a" (result) : "dN" (port));
	return result;
}

} // namespace Sortix

#endif
