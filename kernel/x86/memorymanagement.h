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
 * x86/memorymanagement.h
 * Handles memory for the x64 architecture.
 */

#ifndef SORTIX_X64_MEMORYMANAGEMENT_H
#define SORTIX_X64_MEMORYMANAGEMENT_H

namespace Sortix {
namespace Memory {

const size_t TOPPMLLEVEL = 2;
const size_t ENTRIES = 4096UL / sizeof(addr_t);
const size_t TRANSBITS = 10;

PML* const PMLS[TOPPMLLEVEL + 1] =
{
	(PML* const) 0x0,
	(PML* const) 0xFFC00000UL,
	(PML* const) 0xFFBFF000UL,
};

PML* const FORKPML = (PML* const) 0xFF800000UL;

} // namespace Memory
} // namespace Sortix

namespace Sortix {
namespace Page {

addr_t* const STACK = (addr_t* const) 0xFF400000UL;
const size_t MAXSTACKSIZE = (4UL*1024UL*1024UL);
const size_t MAXSTACKLENGTH = MAXSTACKSIZE / sizeof(addr_t);

} // namespace Page
} // namespace Sortix

#endif
