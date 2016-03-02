/*
 * Copyright (c) 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * disk/ahci/ahci.h
 * Driver for the Advanced Host Controller Interface.
 */

#ifndef SORTIX_DISK_AHCI_AHCI_H
#define SORTIX_DISK_AHCI_AHCI_H

#include <endian.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {
namespace AHCI {

bool WaitClear(volatile little_uint32_t* reg,
               uint32_t bits,
               bool any,
               unsigned int msecs);
bool WaitSet(volatile little_uint32_t* reg,
             uint32_t bits,
             bool any,
             unsigned int msecs);
void Init(const char* devpath, Ref<Descriptor> dev);

} // namespace AHCI
} // namespace Sortix

#endif
