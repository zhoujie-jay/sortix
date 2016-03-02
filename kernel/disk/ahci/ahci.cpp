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
 * disk/ahci/ahci.cpp
 * Driver for the Advanced Host Controller Interface.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <timespec.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/time.h>

#include "ahci.h"
#include "hba.h"

namespace Sortix {
namespace AHCI {

bool WaitClear(volatile little_uint32_t* reg,
               uint32_t bits,
               bool any,
               unsigned int msecs)
{
	struct timespec timeout = timespec_make(msecs / 1000, (msecs % 1000) * 1000000L);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	struct timespec begun;
	clock->Get(&begun, NULL);
	while ( true )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		uint32_t reg_snapshop = *reg;
		if ( !any && (reg_snapshop & bits) == 0 )
			return true;
		if ( any && (reg_snapshop & bits) != bits )
			return true;
		struct timespec elapsed = timespec_sub(now, begun);
		if ( timespec_le(timeout, elapsed) )
			return errno = ETIMEDOUT, false;
		kthread_yield();
	}
}

bool WaitSet(volatile little_uint32_t* reg,
             uint32_t bits,
             bool any,
             unsigned int msecs)
{
	struct timespec timeout = timespec_make(msecs / 1000, (msecs % 1000) * 1000000L);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	struct timespec begun;
	clock->Get(&begun, NULL);
	while ( true )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		uint32_t reg_snapshop = *reg;
		if ( !any && (reg_snapshop & bits) == bits )
			return true;
		if ( any && (reg_snapshop & bits) != 0 )
			return true;
		struct timespec elapsed = timespec_sub(now, begun);
		if ( timespec_le(timeout, elapsed) )
			return errno = ETIMEDOUT, false;
		kthread_yield();
	}
}

static void InitializeDevice(Ref<Descriptor> dev, const char* devpath,
                             uint32_t devaddr)
{
	HBA* hba = new HBA(devaddr);
	if ( !hba )
		PanicF("Failed to allocate ATA driver for AHCI device 0x%X", devaddr);

	if ( !hba->Initialize(dev, devpath) )
		return delete hba;
}

void Init(const char* devpath, Ref<Descriptor> dev)
{
	uint32_t devaddr;
	pcifind_t filter;

	memset(&filter, 255, sizeof(filter));
	filter.classid = 0x01;
	filter.subclassid = 0x06;

	devaddr = 0;
	while ( (devaddr = PCI::SearchForDevices(filter, devaddr)) )
		InitializeDevice(dev, devpath, devaddr);
}

} // namespace AHCI
} // namespace Sortix
