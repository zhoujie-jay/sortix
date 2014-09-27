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

    disk/ata/hba.h
    Driver for ATA.

*******************************************************************************/

#ifndef SORTIX_DISK_ATA_HBA_H
#define SORTIX_DISK_ATA_HBA_H

#include <stdint.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kthread.h>

namespace Sortix {
namespace ATA {

class HBA;
class Port;

class Channel
{
	friend class Port;
	friend class HBA;

public:
	Channel();
	~Channel();

public:
	bool Initialize(Ref<Descriptor> dev, const char* devpath);
	void OnInterrupt();

private:
	__attribute__((format(printf, 2, 3)))
	void LogF(const char* format, ...);
	void SelectDrive(unsigned int drive_index);

private:
	struct interrupt_handler interrupt_registration;
	kthread_mutex_t hw_lock;
	uint32_t devaddr;
	HBA* hba;
	unsigned int channel_index;
	unsigned int current_drive;
	uint16_t port_base;
	uint16_t port_control;
	uint16_t busmaster_base;
	Port* drives[2];
	unsigned int interrupt_index;
	bool interrupt_registered;

};

class HBA
{
	friend class Port;

public:
	HBA(uint32_t devaddr);
	~HBA();

public:
	bool Initialize(Ref<Descriptor> dev, const char* devpath);

private:
	bool InitializeChannel(Ref<Descriptor> dev, const char* devpath,
	                       unsigned int channel_index);

private:
	uint32_t devaddr;
	Channel channels[2];

};

} // namespace ATA
} // namespace Sortix

#endif
