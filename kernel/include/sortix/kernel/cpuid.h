/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/cpuid.h
    Interface to using the cpuid instruction.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_CPUID_H
#define INCLUDE_SORTIX_KERNEL_CPUID_H

#include <stdint.h>

namespace Sortix {

#if defined(__i386__) || defined(__x86_64__)

__attribute__((const))
__attribute__((unused))
static inline bool IsCPUIdSupported()
{
#if defined(__x86_64__)
	// TODO: Isn't this always supported under x86_64?
	uint64_t id_supported;
	asm ("pushfq\n\t"
	     "popq %%rax             /* Get EFLAGS into EAX */\n\t"
	     "movq %%rax, %%rcx      /* Save original flags in ECX */\n\t"
	     "xorq $0x200000, %%rax  /* Flip ID bit in EFLAGS */\n\t"
	     "pushq %%rax            /* Store modified EFLAGS on stack */\n\t"
	     "popfq                  /* Replace current EFLAGS */\n\t"
	     "pushfq                 /* Read back the EFLAGS */\n\t"
	     "popq %%rax             /* Get EFLAGS into EAX */\n\t"
	     "xorq %%rcx, %%rax      /* Check if flag could be modified */\n\t"
	     : "=a" (id_supported)
	     : /* No inputs.  */
	     : /* Clobbered:  */ "%rcx");
#elif  defined(__i386__)
	uint32_t id_supported;
	asm ("pushfl\n\t"
	     "popl %%eax             /* Get EFLAGS into EAX */\n\t"
	     "movl %%eax, %%ecx      /* Save original flags in ECX */\n\t"
	     "xorl $0x200000, %%eax  /* Flip ID bit in EFLAGS */\n\t"
	     "pushl %%eax            /* Store modified EFLAGS on stack */\n\t"
	     "popfl                  /* Replace current EFLAGS */\n\t"
	     "pushfl                 /* Read back the EFLAGS */\n\t"
	     "popl %%eax             /* Get EFLAGS into EAX */\n\t"
	     "xorl %%ecx, %%eax      /* Check if flag could be modified */\n\t"
	     : "=a" (id_supported)
	     : /* No inputs.  */
	     : /* Clobbered:  */ "%rcx");
#endif
	return id_supported != 0;
}

#define cpuid(num,a,b,c,d) \
  asm volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
                : "=a" (a), "=r" (b), "=c" (c), "=d" (d)  \
                : "0" (num))

#endif

} // namespace Sortix

#endif
