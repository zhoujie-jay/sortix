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

    disk/ahci/hba.h
    Driver for the Advanced Host Controller Interface.

*******************************************************************************/

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
