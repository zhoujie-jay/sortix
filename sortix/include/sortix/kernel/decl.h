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

    sortix/kernel/decl.h
    Various declarations. These should likely be replaced with better names from
    standard headers or at least one with a less generic name than decl.h.

*******************************************************************************/

#ifndef SORTIX_DECL_H
#define SORTIX_DECL_H

typedef uintptr_t addr_t;

#define SORTIX_NORETURN __attribute__((noreturn))
#define SORTIX_MAYALIAS __attribute__((__may_alias__))
#define SORTIX_PACKED __attribute__((packed))
#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#define STATIC_ASSERT(condition) static_assert(condition, #condition)

#if !defined(CPU) && defined(__i386__)
	#define CPU X86
#endif

#if !defined(CPU) && defined(__x86_64__)
	#define CPU X64
#endif

#endif
