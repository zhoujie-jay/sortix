/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	decl.h
	Various declarations. These should likely be replaced with better names from
	standard headers or at least one with a less generic name than decl.h.

*******************************************************************************/

#ifndef SORTIX_DECL_H
#define SORTIX_DECL_H

#ifndef _ADDR_T_DECLARED
#define _ADDR_T_DECLARED
typedef uintptr_t addr_t;
#endif

#define SORTIX_NORETURN __attribute__((noreturn))
#define SORTIX_MAYALIAS __attribute__((__may_alias__))
#define SORTIX_PACKED __attribute__((packed))
#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#define STATIC_ASSERT(condition) static_assert(condition, #condition)

#define ASSERT(invariant) \
	if ( unlikely(!(invariant)) ) \
	{ \
		Sortix::PanicF("Assertion failure: %s:%u: %s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #invariant); \
		while ( true ) { } \
	}

// The following declarations should not be used if possible. They were part of
// what libmaxsi's old platform.h header declared and the kernel continues to
// depend on it.
#if !defined(PLATFORM_X64) && defined(__x86_64__)
	#define PLATFORM_X64
#elif !defined(PLATFORM_X86) && defined(__i386__)
	#define PLATFORM_X86
#endif

#if !defined(CPU) && defined(PLATFORM_X86)
	#define CPU X86
#endif

#if !defined(CPU) && defined(PLATFORM_X64)
	#define CPU X64
#endif

#if !defined(CPU_FAMILY) && defined(PLATFORM_X86) || defined(PLATFORM_X64)
	#define PLATFORM_X86_FAMILY
	#define CPU_FAMILY X86_FAMILY
#endif

#endif
