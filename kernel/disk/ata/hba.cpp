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
 * disk/ata/hba.cpp
 * Driver for ATA.
 */

#include <errno.h>
#include <stdarg.h>
#include <timespec.h>

#include <sortix/clock.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/time.h>

#include "../node.h"

#include "hba.h"
#include "port.h"
#include "registers.h"

namespace Sortix {
namespace ATA {

static unsigned long AllocateDiskNumber()
{
	static unsigned long next_disk_number = 0;
	return InterlockedIncrement(&next_disk_number).o;
}

static void sleep_400_nanoseconds()
{
	struct timespec delay = timespec_make(0, 400);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	clock->SleepDelay(delay);
}

Channel::Channel()
{
	hw_lock = KTHREAD_MUTEX_INITIALIZER;
	interrupt_registered = false;
	drives[0] = NULL;
	drives[1] = NULL;
}

Channel::~Channel()
{
	if ( interrupt_registered )
		Interrupt::UnregisterHandler(interrupt_index, &interrupt_registration);
	// TODO: Destroy all the ports?
}

void Channel::LogF(const char* format, ...)
{
	// TODO: Print this line in an atomic manner.
	const char* cdesc = channel_index == 0 ? "primary" : "secondary";
	Log::PrintF("ata: pci 0x%X: %s channel: ", devaddr, cdesc);
	va_list ap;
	va_start(ap, format);
	Log::PrintFV(format, ap);
	va_end(ap);
	Log::PrintF("\n");
}

void Channel::SelectDrive(unsigned int drive_index) // hw_lock locked
{
	if ( current_drive == drive_index )
		return;

#if 0
	// TODO: Perhaps not do this here. This appears to time out on boot on real
	//       hardware where there is no drive there.
	if ( !wait_inport8_clear(port_base + REG_STATUS,
	                         STATUS_BUSY | STATUS_DATAREADY, false, 10000 /*ms*/) )
	{
		LogF("error: timed out waiting for idle");
		// TODO: This can fail!
		//return errno = EIO, false;
		return;
	}
#endif

	uint8_t value = 0xA0 | (drive_index << 4);
	outport8(port_base + REG_DRIVE_SELECT, value);
	//outport8(port_control, value); // TODO: Or is it port_control we use?

	sleep_400_nanoseconds();

	// TODO: Do we need to wait for non-busy now? Can this operation fail?

	current_drive = drive_index;
}

void Channel::OnInterrupt()
{
	// Check whether the interrupt came from this channel.
	uint8_t status = inport8(busmaster_base + BUSMASTER_REG_STATUS);
	if ( !(status & BUSMASTER_STATUS_INTERRUPT_PENDING) )
		return;

	if ( status & BUSMASTER_STATUS_DMA_FAILURE )
	{
		// TODO: What do we do here?
	}

	// Clear interrupt flag.
	// TODO: This filters away BUSMASTER_STATUS_DMA and
	//       BUSMASTER_STATUS_DMA_FAILURE and might be a bug?
	//status &= 0xF8 | BUSMASTER_STATUS_DMA;
	status |= BUSMASTER_STATUS_INTERRUPT_PENDING;
	outport8(busmaster_base + BUSMASTER_REG_STATUS, status);

	if ( current_drive < 2 && drives[current_drive] )
		drives[current_drive]->OnInterrupt();
}

void Channel__OnInterrupt(struct interrupt_context*, void* context)
{
	((Channel*) context)->OnInterrupt();
}

static
void FixDefaultDeviceBars(pcibar_t* basebar, pcibar_t* ctrlbar, uint8_t* irq,
                          unsigned int channel_index, uint8_t interface)
{
	bool compatibility = interface == 0x00 || interface == 0x02;

	if ( compatibility )
		*irq = channel_index == 0 ? 14 : 15;

	if ( compatibility ||
	     basebar->addr_raw == 0 ||
	     (basebar->is_iospace() && !ctrlbar->ioaddr()) )
	{
		uint16_t ioport = channel_index == 0 ? 0x1F0 : 0x170;
		basebar->addr_raw = ioport | PCIBAR_TYPE_IOSPACE;
		basebar->size_raw = 8;
	}

	if ( compatibility ||
	     ctrlbar->addr_raw == 0 ||
	     (ctrlbar->is_iospace() && !ctrlbar->ioaddr()) )
	{
		uint16_t ioport = channel_index == 0 ? 0x3F4 : 0x374;
		ctrlbar->addr_raw = ioport | PCIBAR_TYPE_IOSPACE;
		ctrlbar->size_raw = 4; // TODO: This is just a guess.
	}
}

bool Channel::Initialize(Ref<Descriptor> dev, const char* devpath)
{
	uint8_t prog_if = PCI::Read8(devaddr, PCIFIELD_PROG_IF);
	uint8_t interface = (prog_if >> (channel_index * 2)) & 0x3;
	pcibar_t basebar = PCI::GetBAR(devaddr, 2 * channel_index + 0);
	pcibar_t ctrlbar = PCI::GetBAR(devaddr, 2 * channel_index + 1);
	pcibar_t busmasterbar = PCI::GetBAR(devaddr, 4);
	uint8_t irq = PCI::Read8(devaddr, PCIFIELD_INTERRUPT_LINE);
	FixDefaultDeviceBars(&basebar, &ctrlbar, &irq, channel_index, interface);

	if ( !basebar.is_iospace() )
	{
		LogF("ignoring: non-iospace base BAR");
		return errno = EINVAL, false;
	}

	if ( basebar.size() < 8 )
	{
		LogF("ignoring: too small base BAR");
		return errno = EINVAL, false;
	}

	if ( !ctrlbar.is_iospace() )
	{
		LogF("ignoring: non-iospace control BAR");
		return errno = EINVAL, false;
	}

	if ( ctrlbar.size() < 4 )
	{
		LogF("ignoring: too small control BAR");
		return errno = EINVAL, false;
	}

	if ( !busmasterbar.is_iospace() )
	{
		LogF("ignoring: non-iospace bus master BAR");
		return errno = EINVAL, false;
	}

	if ( busmasterbar.size() < 16 )
	{
		LogF("ignoring: too small bus master BAR");
		return errno = EINVAL, false;
	}

	port_base = basebar.addr();
	if ( inport8(port_base + REG_STATUS) == 0xFF )
		return errno = ENODEV, false; // Non-existent.

	// TODO: Ensure this is the correct logic.
	port_control = ctrlbar.addr() + 2;

	busmaster_base = busmasterbar.addr() + 8 * channel_index;

	current_drive = (unsigned int) -1; // We don't know.

	for ( unsigned int i = 0; i < 2; i++ )
	{
		drives[i] = NULL;

		ScopedLock lock(&hw_lock);

		SelectDrive(i);

		// TODO: May we do this before sending an IDENTITY command?
		uint8_t status = inport8(port_base + REG_STATUS);
		if ( status == 0 )
			continue; // Non-existent.

		const char* name = i == 0 ? "master" : "slave";

		drives[i] = new Port(this, i);
		if ( !drives[i] )
		{
			LogF("error: failed to allocate %s drive", name);
			continue;
		}

		if ( !drives[i]->Initialize() )
		{
			delete drives[i];
			drives[i] = NULL;
			continue;
		}
	}

	interrupt_index = Interrupt::IRQ0 + irq;

	interrupt_registration.handler = Channel__OnInterrupt;
	interrupt_registration.context = this;
	Interrupt::RegisterHandler(interrupt_index, &interrupt_registration);
	interrupt_registered = true;

	for ( unsigned int i = 0; i < 2; i++ )
	{
		if ( !drives[i] )
			continue;
		if ( !drives[i]->FinishInitialize() )
		{
			// TODO: Gracefully destroy the drive here?
			// TODO: Unsafe with respect to interrupt handler.
			delete drives[i];
			drives[i] = NULL;
			continue;
		}
	}

	for ( unsigned int i = 0; i < 2; i++ )
	{
		if ( !drives[i] )
			continue;
		unsigned long number = AllocateDiskNumber();
		char name[3 + sizeof(unsigned long) * 3];
		snprintf(name, sizeof(name), "ata%lu", number);
		Ref<PortNode> node(new PortNode(drives[i], 0, 0, 0660, dev->dev, 0));
		if ( !node )
			PanicF("Unable to allocate memory for %s/%s inode", devpath, name);
		ioctx_t ctx; SetupKernelIOCtx(&ctx);
		if ( LinkInodeInDir(&ctx, dev, name, node) != 0 )
			PanicF("Unable to link %s/%s to ATA driver inode", devpath, name);
	}

	return true;
}

HBA::HBA(uint32_t devaddr)
{
	this->devaddr = devaddr;
}

HBA::~HBA()
{
}

bool HBA::InitializeChannel(Ref<Descriptor> dev, const char* devpath,
                            unsigned int channel_index)
{
	channels[channel_index].devaddr = devaddr;
	channels[channel_index].hba = this;
	channels[channel_index].channel_index = channel_index;
	return channels[channel_index].Initialize(dev, devpath);
}

bool HBA::Initialize(Ref<Descriptor> dev, const char* devpath)
{
	uint16_t dev_pci_command_cur = PCI::Read16(devaddr, PCIFIELD_COMMAND);
	uint16_t dev_pci_command_new = dev_pci_command_cur;
	dev_pci_command_new &= ~PCIFIELD_COMMAND_INTERRUPT_DISABLE;
	dev_pci_command_new |= PCIFIELD_COMMAND_IO_SPACE;
	dev_pci_command_new |= PCIFIELD_COMMAND_BUS_MASTER;
	if ( dev_pci_command_cur != dev_pci_command_new )
		PCI::Write16(devaddr, PCIFIELD_COMMAND, dev_pci_command_new);

	InitializeChannel(dev, devpath, 0);
	InitializeChannel(dev, devpath, 1);

	return true;
}

} // namespace ATA
} // namespace Sortix
