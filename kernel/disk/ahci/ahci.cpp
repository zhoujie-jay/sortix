/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015, 2016.

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

    disk/ahci/ahci.cpp
    Driver for the Advanced Host Controller Interface.

*******************************************************************************/

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
