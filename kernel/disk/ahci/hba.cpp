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
 * disk/ahci/hba.cpp
 * Driver for the Advanced Host Controller Interface.
 */

#include <errno.h>
#include <stdarg.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/pci-mmio.h>

#include "../node.h"

#include "ahci.h"
#include "hba.h"
#include "port.h"
#include "registers.h"

namespace Sortix {
namespace AHCI {

static inline void ahci_hba_flush(volatile struct hba_regs* hba_regs)
{
	(void) hba_regs->ghc;
}

static unsigned long AllocateDiskNumber()
{
	static unsigned long next_disk_number = 0;
	return InterlockedIncrement(&next_disk_number).o;
}

HBA::HBA(uint32_t devaddr)
{
	this->devaddr = devaddr;
	memset(&mmio_alloc, 0, sizeof(mmio_alloc));
	memset(&ports, 0, sizeof(ports));
	interrupt_registered = false;
	mmio_alloced = false;
}

HBA::~HBA()
{
	// TODO: Destroy all the ports?
	if ( interrupt_registered )
		Interrupt::UnregisterHandler(interrupt_index, &interrupt_registration);
	if ( mmio_alloced )
		UnmapPCIBar(&mmio_alloc);
}

void HBA::LogF(const char* format, ...)
{
	// TODO: Print this line in an atomic manner.
	Log::PrintF("ahci: pci 0x%X: ", devaddr);
	va_list ap;
	va_start(ap, format);
	Log::PrintFV(format, ap);
	va_end(ap);
	Log::PrintF("\n");
}

bool HBA::Reset()
{
#if 0 // We probably don't want to do this?
	// Indicate that system software is AHCI aware.
	regs->ghc = regs->ghc | GHC_AE;

	regs->ghc = regs->ghc | GHC_HR;
	ahci_hba_flush(regs);

	while ( (regs->ghc & GHC_HR) )
		kthread_yield();

	regs->ghc = regs->ghc | GHC_AE;
	ahci_hba_flush(regs);

	// TODO: Set port enable bits in Port Control and Status on Intel
	//       controllers.
#endif

	return true;
}

void HBA::OnInterrupt()
{
	uint32_t is = regs->is;
	if ( !is )
		return;

	for ( size_t i = 0; i < sizeof(ports) / sizeof(ports[0]); i++ )
	{
		if ( !(is & (1U << i)) )
			continue;
		if ( ports[i] )
			ports[i]->OnInterrupt();
	}

	regs->is = is;
}

void HBA__OnInterrupt(struct interrupt_context*, void* context)
{
	((HBA*) context)->OnInterrupt();
}

bool HBA::Initialize(Ref<Descriptor> dev, const char* devpath)
{
	pcibar_t mmio_bar = PCI::GetBAR(devaddr, 5);
	if ( mmio_bar.size() < sizeof(struct hba_regs) && /* or || ? */
	     mmio_bar.size() < 1024 )
	{
		LogF("error: register area is too small");
		return errno = EINVAL, false;
	}

	if ( !MapPCIBAR(&mmio_alloc, mmio_bar, 0) )
	{
		LogF("error: registers could not be mapped: %m");
		return false;
	}

	regs = (volatile struct hba_regs*) mmio_alloc.from;

	if ( 4 < sizeof(void*) && !(regs->cap & CAP_S64A) )
	{
		LogF("error: controller doesn't support 64-bit addressing");
		return errno = EINVAL, false;
	}

	// TODO: Take control from BIOS?

#if 0
	// TODO: Is it bad to do a hard reset?
	if ( !Reset() )
	{
		LogF("error: could not reset");
		return false;
	}
#else
	regs->ghc = regs->ghc | GHC_AE;
#endif

	regs->ghc = regs->ghc & ~GHC_IE;

	uint32_t ports_implemented = regs->pi;
	uint32_t num_ports = CAP_NCS(regs->cap);

	uint32_t actual_ports = 0;

	for ( uint32_t i = 0; i < num_ports; i++ )
	{
		if ( !(ports_implemented & (1 << i)) )
			continue;

		actual_ports++;

		if ( regs->ports[i].pxcmd & (PXCMD_ST | PXCMD_CR |
		                             PXCMD_FRE | PXCMD_FR) )
		{
			if ( regs->ports[i].pxcmd & (PXCMD_ST | PXCMD_CR) )
			{
				regs->ports[i].pxcmd = regs->ports[i].pxcmd & ~PXCMD_ST;
				if ( !WaitClear(&regs->ports[i].pxcmd, PXCMD_CR, true, 500) )
				{
					LogF("error: port %u: "
					     "timeout waiting for PXCMD_CR to clear", i);
					ports_implemented &= ~(1 << i);
					continue;
				}
			}

			if ( regs->ports[i].pxcmd & (PXCMD_FRE | PXCMD_FR) )
			{
				regs->ports[i].pxcmd = regs->ports[i].pxcmd & ~PXCMD_FRE;
				if ( !WaitClear(&regs->ports[i].pxcmd, PXCMD_FR, true, 500) )
				{
					LogF("error: port %u: "
					     "timeout waiting for PXCMD_FR to clear", i);
					ports_implemented &= ~(1 << i);
					continue;
				}
			}
		}

		regs->ports[i].pxis = regs->ports[i].pxis;
		regs->ports[i].pxserr = regs->ports[i].pxserr;
	}

	regs->is = regs->is;

	for ( uint32_t i = 0; i < num_ports; i++ )
	{
		if ( !(ports_implemented & (1 << i)) )
			continue;

		if ( !(ports[i] = new Port(this, i)) )
		{
			LogF("error: failed to allocate port %u", i);
			continue;
		}

		if ( !ports[i]->Initialize() )
		{
			delete ports[i];
			ports[i] = NULL;
			continue;
		}
	}

	interrupt_index =
		Interrupt::IRQ0 + PCI::Read8(devaddr, PCIFIELD_INTERRUPT_LINE);
	interrupt_registration.handler = HBA__OnInterrupt;
	interrupt_registration.context = this;
	Interrupt::RegisterHandler(interrupt_index, &interrupt_registration);
	interrupt_registered = true;

	// Enable interrupts.
	regs->ghc = regs->ghc | GHC_IE;
	ahci_hba_flush(regs);

	for ( uint32_t i = 0; i < num_ports; i++ )
	{
		if ( ports[i] && !ports[i]->FinishInitialize() )
		{
			//ports[i]->Destroy();
			// TODO: WARNING: FIXME: Shut down the port here, see the function
			//                       ahci_port_destroy in the OS `kiwi'!
			// TODO: Unsafe with respect to interrupt handler.
			delete ports[i];
			ports[i] = NULL;
			continue;
		}
	}

	for ( uint32_t i = 0; i < num_ports; i++ )
	{
		if ( !ports[i] )
			continue;
		unsigned long number = AllocateDiskNumber();
		char name[4 + sizeof(unsigned long) * 3];
		snprintf(name, sizeof(name), "ahci%lu", number);
		Ref<PortNode> node(new PortNode(ports[i], 0, 0, 0660, dev->dev, 0));
		if ( !node )
			PanicF("Unable to allocate memory for %s/%s inode", devpath, name);
		ioctx_t ctx; SetupKernelIOCtx(&ctx);
		if ( LinkInodeInDir(&ctx, dev, name, node) != 0 )
			PanicF("Unable to link %s/%s to AHCI driver inode", devpath, name);
	}

	return true;
}

} // namespace AHCI
} // namespace Sortix
