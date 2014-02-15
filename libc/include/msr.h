/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    msr.h
    High-level access to the rdmsr and wrmsr instructions for accessing and
    modifying machine specific registers.

*******************************************************************************/

#ifndef INCLUDE_MSR_H
#define INCLUDE_MSR_H

#include <sys/cdefs.h>

#include <stdint.h>

#if defined(__i386__) || defined(__x86_64__)

__BEGIN_DECLS

__attribute__((unused))
static inline uint64_t rdmsr(uint32_t msrid)
{
	uint32_t low;
	uint32_t high;
	asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msrid));
	return (uint64_t) low << 0 | (uint64_t) high << 32;
}

__attribute__((unused))
static inline uint64_t wrmsr(uint32_t msrid, uint64_t value)
{
	uint32_t low = value >> 0 & 0xFFFFFFFF;
	uint32_t high = value >> 32 & 0xFFFFFFFF;;
	asm volatile ("wrmsr" : : "a"(low), "d"(high), "c"(msrid) : "memory");
	return value;
}

__attribute__((unused))
static inline void rdmsr_split(uint32_t msrid, uint32_t* low, uint32_t* high)
{
	uint64_t result = rdmsr(msrid);
	*low = result >> 0 & 0xFFFFFFFF;
	*high = result >> 32 & 0xFFFFFFFF;
}

__attribute__((unused))
static inline void wrmsr_split(uint32_t msrid, uint32_t low, uint32_t high)
{
	wrmsr(msrid, (uint64_t) low << 0 | (uint64_t) high << 32);
}

__END_DECLS

#endif

#endif