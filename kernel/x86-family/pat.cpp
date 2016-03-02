/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
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
 * x86-family/pat.cpp
 * Tests whether PAT is available and initializes it.
 */

#include <msr.h>
#include <stdint.h>

#include <sortix/kernel/cpuid.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/pat.h>

#include "memorymanagement.h"

namespace Sortix {

static const uint32_t bit_PAT = 0x00010000U;

bool IsPATSupported()
{
	if ( !IsCPUIdSupported() )
		return false;
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	uint32_t features = edx;
	return features & bit_PAT;
}

void InitializePAT()
{
	using namespace Sortix::Memory;
	const uint32_t LOW = PA[0] << 0 | PA[1] << 8 | PA[2] << 16 | PA[3] << 24;
	const uint32_t HIGH = PA[4] << 0 | PA[5] << 8 | PA[6] << 16 | PA[7] << 24;
	const int PAT_REG = 0x0277;
	wrmsr(PAT_REG, (uint64_t) LOW << 0 | (uint64_t) HIGH << 32);
}

} // namespace Sortix
