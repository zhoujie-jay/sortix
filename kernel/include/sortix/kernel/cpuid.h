/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/cpuid.h
 * Interface to using the cpuid instruction.
 */

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
