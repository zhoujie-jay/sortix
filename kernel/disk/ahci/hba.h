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
 * disk/ahci/hba.h
 * Driver for the Advanced Host Controller Interface.
 */

#ifndef SORTIX_DISK_AHCI_HBA_H
#define SORTIX_DISK_AHCI_HBA_H

#include <stdint.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/interrupt.h>

namespace Sortix {
namespace AHCI {

struct hba_regs;

class Port;

class HBA
{
	friend class Port;

public:
	HBA(uint32_t devaddr);
	~HBA();

public:
	bool Initialize(Ref<Descriptor> dev, const char* devpath);
	void OnInterrupt();

private:
	__attribute__((format(printf, 2, 3)))
	void LogF(const char* format, ...);
	bool Reset();

private:
	struct interrupt_handler interrupt_registration;
	Port* ports[32];
	addralloc_t mmio_alloc;
	volatile struct hba_regs* regs;
	uint32_t devaddr;
	unsigned int interrupt_index;
	bool interrupt_registered;
	bool mmio_alloced;

};

} // namespace AHCI
} // namespace Sortix

#endif
