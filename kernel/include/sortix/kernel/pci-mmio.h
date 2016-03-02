/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/pci-mmio.h
 * Functions for handling PCI devices.
 */

#ifndef INCLUDE_SORTIX_KERNEL_PCI_MMIO_H
#define INCLUDE_SORTIX_KERNEL_PCI_MMIO_H

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/pci.h>

namespace Sortix {

const int MAP_PCI_BAR_WRITE_COMBINE = 1 << 0;

bool MapPCIBAR(addralloc_t* allocation, pcibar_t bar, int flags = 0);
void UnmapPCIBar(addralloc_t* allocation);

} // namespace Sortix

#endif
