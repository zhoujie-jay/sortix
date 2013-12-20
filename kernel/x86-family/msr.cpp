/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.
    Copyright(C) Free Software Foundation, Inc. 2005, 2006, 2007, 2008, 2009.

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

    x86-family/msr.cpp
    Functions to manipulate Model Specific Registers. MTRR code is partially
    based on code from GNU GRUB.

*******************************************************************************/

#include <sortix/kernel/kernel.h>

#include "memorymanagement.h"

namespace Sortix {
namespace MSR {

const uint32_t bit_MTRR = 0x00001000U;
const uint32_t bit_PAT  = 0x00010000U;

// TODO: Move this to a better location or use <cpuid.h>.
static inline bool IsCPUIdSupported()
{
#ifdef __x86_64__
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
#else
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

#define rdmsr(num,a,d) \
  asm volatile ("rdmsr" : "=a" (a), "=d" (d) : "c" (num))

#define wrmsr(num,lo,hi) \
  asm volatile ("wrmsr" : : "c" (num), "a" (lo), "d" (hi) : "memory")

#define mtrr_base(reg) (0x200 + (reg) * 2)
#define mtrr_mask(reg) (0x200 + (reg) * 2 + 1)

void EnableMTRR(int mtrr)
{
	uint32_t eax, edx;
	uint32_t mask_lo, mask_hi;

	rdmsr(mtrr_mask(mtrr), eax, edx);
	mask_lo = eax;
	mask_hi = edx;

	mask_lo |= 0x800 /* valid */;
	wrmsr(mtrr_mask(mtrr), mask_lo, mask_hi);
}

void DisableMTRR(int mtrr)
{
	uint32_t eax, edx;
	uint32_t mask_lo, mask_hi;

	rdmsr(mtrr_mask(mtrr), eax, edx);
	mask_lo = eax;
	mask_hi = edx;

	mask_lo &= ~0x800 /* valid */;
	wrmsr(mtrr_mask(mtrr), mask_lo, mask_hi);
}

void CopyMTRR(int dst, int src)
{
	uint32_t base_lo, base_hi;
	uint32_t mask_lo, mask_hi;
	rdmsr(mtrr_base(src), base_lo, base_hi);
	rdmsr(mtrr_mask(src), mask_lo, mask_hi);
	wrmsr(mtrr_base(dst), base_lo, base_hi);
	wrmsr(mtrr_mask(dst), mask_lo, mask_hi);
}

bool IsPATSupported()
{
	if ( !IsCPUIdSupported() ) { return false; }
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	uint32_t features = edx;
	return features & bit_PAT;
}

void InitializePAT()
{
	using namespace Sortix::Memory;
	const uint32_t LO = PA[0] << 0 | PA[1] << 8 | PA[2] << 16 | PA[3] << 24;
	const uint32_t HI = PA[4] << 0 | PA[5] << 8 | PA[6] << 16 | PA[7] << 24;
	const int PAT_REG = 0x0277;
	wrmsr(PAT_REG, LO, HI);
}

bool IsMTRRSupported()
{
	if ( !IsCPUIdSupported() ) { return false; }
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	uint32_t features = edx;
	return features & bit_MTRR;
}

// TODO: Yes, returning a string as an error and giving the result in a pointer
// is very bad design. Please fix this at some point. Also improve the code such
// that it is more flexible.
const char* SetupMTRRForWC(addr_t base, size_t size, int* ret)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t mtrrcap;
	int var_mtrrs;
	uint32_t max_extended_cpuid;
	uint32_t maxphyaddr;
	uint64_t fb_base, fb_size;
	uint64_t size_bits, fb_mask;
	uint32_t bits_lo, bits_hi;
	uint64_t bits;
	int i, first_unused = -1;
	uint32_t base_lo, base_hi, mask_lo, mask_hi;

	fb_base = (uint64_t) base;
	fb_size = (uint64_t) size;

	// Check that fb_base and fb_size can be represented using a single MTRR.

	if ( fb_base < (1 << 20) )
		return "below 1MB, so covered by fixed-range MTRRs";
	if ( fb_base >= (1LL << 36) )
		return "over 36 bits, so out of range";
	if ( fb_size < (1 << 12) )
		return "variable-range MTRRs must cover at least 4KB";

	size_bits = fb_size;
	while ( size_bits > 1 )
		size_bits >>= 1;
	if ( size_bits != 1 )
		return "not a power of two";

	if ( fb_base & (fb_size - 1) )
		return "not aligned on size boundary";

	fb_mask = ~(fb_size - 1);

	// Check CPU capabilities.

	if ( !IsCPUIdSupported() )
		return "cpuid not supported, therefore mtrr not supported";

	if ( !IsMTRRSupported() )
		return "cpu does not support mtrr";

	rdmsr(0xFE, eax, edx);
	mtrrcap = eax;
	if ( !(mtrrcap & 0x00000400) ) /* write-combining */
		return "write combining doesn't seem to be supported";
	var_mtrrs = (mtrrcap & 0xFF);

	cpuid (0x80000000, eax, ebx, ecx, edx);
	max_extended_cpuid = eax;
	if ( max_extended_cpuid >= 0x80000008 )
	{
		cpuid(0x80000008, eax, ebx, ecx, edx);
		maxphyaddr = (eax & 0xFF);
	}
	else
		maxphyaddr = 36;
	bits_lo = 0xFFFFF000; /* assume maxphyaddr >= 36 */
	bits_hi = (1 << (maxphyaddr - 32)) - 1;
	bits = bits_lo | ((uint64_t) bits_hi << 32);

	// Check whether an MTRR already covers this region. If not, take an unused
	// one if possible.
	for ( i = 0; i < var_mtrrs; i++ )
	{
		rdmsr(mtrr_mask (i), eax, edx);
		mask_lo = eax;
		mask_hi = edx;
		if ( mask_lo & 0x800 ) /* valid */
		{
			uint64_t real_base, real_mask;

			rdmsr(mtrr_base(i), eax, edx);
			base_lo = eax;
			base_hi = edx;

			real_base = ((uint64_t) (base_hi & bits_hi) << 32) |
			            (base_lo & bits_lo);
			real_mask = ((uint64_t) (mask_hi & bits_hi) << 32) |
			            (mask_lo & bits_lo);

			if ( real_base < (fb_base + fb_size) &&
			     real_base + (~real_mask & bits) >= fb_base)
				return "region already covered by another mtrr";
		}
		else if ( first_unused < 0 )
			first_unused = i;
	}

	if ( first_unused < 0 )
		return "all MTRRs in use";

	// Set up the first unused MTRR we found.
	rdmsr(mtrr_base(first_unused), eax, edx);
	base_lo = eax;
	base_hi = edx;
	rdmsr(mtrr_mask(first_unused), eax, edx);
	mask_lo = eax;
	mask_hi = edx;

	base_lo = (base_lo & ~bits_lo & ~0xFF) |
	          (fb_base & bits_lo) | 0x01 /* WC */;
	base_hi = (base_hi & ~bits_hi) |
	          ((fb_base >> 32) & bits_hi);
	wrmsr(mtrr_base(first_unused), base_lo, base_hi);
	mask_lo = (mask_lo & ~bits_lo) |
	          (fb_mask & bits_lo) | 0x800 /* valid */;
	mask_hi = (mask_hi & ~bits_hi) |
	          ((fb_mask >> 32) & bits_hi);
	wrmsr(mtrr_mask(first_unused), mask_lo, mask_hi);

	if ( ret )
		*ret = first_unused;

	return NULL;
}

} // namespace MSR
} // namespace Sortix
