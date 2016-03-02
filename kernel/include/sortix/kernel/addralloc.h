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
 * sortix/kernel/addralloc.h
 * Class to keep track of mount points.
 */

#ifndef INCLUDE_SORTIX_KERNEL_ADDRALLOC_H
#define INCLUDE_SORTIX_KERNEL_ADDRALLOC_H

#include <stddef.h>

#include <sortix/kernel/decl.h>

namespace Sortix {

struct addralloc_t
{
	addr_t from;
	size_t size;
};

bool AllocateKernelAddress(addralloc_t* ret, size_t size);
void FreeKernelAddress(addralloc_t* alloc);
size_t ExpandHeap(size_t increase);
void ShrinkHeap(size_t decrease);
addr_t GetHeapLower();
addr_t GetHeapUpper();
size_t GetHeapSize();

} // namespace Sortix

#endif
