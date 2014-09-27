/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015, 2016.

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

    disk/ata/ata.cpp
    Driver for ATA.

*******************************************************************************/

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
