/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * disk/ata/ata.cpp
 * Driver for ATA.
 */

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/refcount.h>

#include "ata.h"
#include "hba.h"

namespace Sortix {
namespace ATA {

static void InitializeDevice(Ref<Descriptor> dev, const char* devpath,
                             uint32_t devaddr)
{
	HBA* hba = new HBA(devaddr);
	if ( !hba )
		PanicF("Failed to allocate ATA driver for PCI device 0x%X", devaddr);

	if ( !hba->Initialize(dev, devpath) )
		return delete hba;
}


void Init(const char* devpath, Ref<Descriptor> dev)
{
	uint32_t devaddr;
	pcifind_t filter;

	memset(&filter, 255, sizeof(filter));
	filter.classid = 0x01;
	filter.subclassid = 0x01;
	devaddr = 0;
	while ( (devaddr = PCI::SearchForDevices(filter, devaddr)) )
		InitializeDevice(dev, devpath, devaddr);
}

} // namespace ATA
} // namespace Sortix
