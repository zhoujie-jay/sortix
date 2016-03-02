/*
 * Copyright (c) 2011, 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * x86-family/memorymanagement.h
 * Handles memory for the x86 family of architectures.
 */

#ifndef SORTIX_X86_FAMILY_MEMORYMANAGEMENT_H
#define SORTIX_X86_FAMILY_MEMORYMANAGEMENT_H

namespace Sortix {

struct PML
{
	addr_t entry[4096 / sizeof(addr_t)];
};

} // namespace Sortix

namespace Sortix {
namespace Memory {

const addr_t PML_PRESENT    = 1 << 0;
const addr_t PML_WRITABLE   = 1 << 1;
const addr_t PML_USERSPACE  = 1 << 2;
const addr_t PML_WRTHROUGH  = 1 << 3;
const addr_t PML_NOCACHE    = 1 << 4;
const addr_t PML_PAT        = 1 << 7;
const addr_t PML_AVAILABLE1 = 1 << 9;
const addr_t PML_AVAILABLE2 = 1 << 10;
const addr_t PML_AVAILABLE3 = 1 << 11;
const addr_t PML_FORK       = PML_AVAILABLE1;
#ifdef __x86_64__
const addr_t PML_NX         = 1UL << 63;
#else
const addr_t PML_NX         = 0;
#endif
const addr_t PML_FLAGS      = 0xFFFUL | PML_NX; // Bits used for the flags.
const addr_t PML_ADDRESS    = ~PML_FLAGS; // Bits used for the address.
const addr_t PAT_UC = 0x00; // Uncacheable
const addr_t PAT_WC = 0x01; // Write-Combine
const addr_t PAT_WT = 0x04; // Writethrough
const addr_t PAT_WP = 0x05; // Write-Protect
const addr_t PAT_WB = 0x06; // Writeback
const addr_t PAT_UCM = 0x07; // Uncacheable, overruled by MTRR.
const addr_t PAT_NUM = 0x08;

// Desired PAT-Register PA-Field Indexing (different from BIOS defaults)
const addr_t PA[PAT_NUM] =
{
	PAT_WB,
	PAT_WT,
	PAT_UCM,
	PAT_UC,
	PAT_WC,
	PAT_WP,
	0,
	0,
};

// Inverse function of the above.
const addr_t PAINV[PAT_NUM] =
{
	3, // UC
	4, // WC
	7, // No such
	8, // No such
	1, // WT
	5, // WP,
	0, // WB
	2, // UCM
};

static inline addr_t EncodePATAsPMLFlag(addr_t pat)
{
	pat = PAINV[pat];
	addr_t result = 0;
	if ( pat & 0x1 ) { result |= PML_WRTHROUGH; }
	if ( pat & 0x2 ) { result |= PML_NOCACHE; }
	if ( pat & 0x4 ) { result |= PML_PAT; }
	return result;
}

bool MapPAT(addr_t physical, addr_t mapto, int prot, addr_t mtype);
addr_t ProtectionToPMLFlags(int prot);
int PMLFlagsToProtection(addr_t flags);

} // namespace Memory
} // namespace Sortix

#if defined(__i386__)
#include "../x86/memorymanagement.h"
#elif defined(__x86_64__)
#include "../x64/memorymanagement.h"
#endif

#endif
