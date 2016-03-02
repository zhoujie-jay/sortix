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
 * disk/ahci/port.cpp
 * Driver for the Advanced Host Controller Interface.
 */

#include <assert.h>
#include <errno.h>
#include <endian.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/mman.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/clock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/time.h>

#include "ahci.h"
#include "hba.h"
#include "port.h"
#include "registers.h"

namespace Sortix {
namespace AHCI {

// TODO: Is this needed?
static inline void ahci_port_flush(volatile struct port_regs* port_regs)
{
	(void) port_regs->pxcmd;
}

static inline void delay(int usecs)
{
	struct timespec delay = timespec_make(usecs / 1000000, usecs * 1000);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	clock->SleepDelay(delay);
}

static void copy_ata_string(char* dest, const char* src, size_t length)
{
	for ( size_t i = 0; i < length; i += 2 )
	{
		dest[i + 0] = src[i + 1];
		dest[i + 1] = src[i + 0];
	}
	length = strnlen(dest, length);
	while ( 0 < length && dest[length - 1] == ' ' )
		length--;
	dest[length] = '\0';
}

Port::Port(HBA* hba, uint32_t port_index)
{
	port_lock = KTHREAD_MUTEX_INITIALIZER;
	memset(&control_alloc, 0, sizeof(control_alloc));
	memset(&dma_alloc, 0, sizeof(dma_alloc));
	this->hba = hba;
	regs = &hba->regs->ports[port_index];
	control_physical_frame = 0;
	dma_physical_frame = 0;
	this->port_index = port_index;
	is_control_page_mapped = false;
	is_dma_page_mapped = false;
	interrupt_signaled = false;
	transfer_in_progress = false;
}

Port::~Port()
{
	if ( transfer_in_progress )
		FinishTransferDMA();
	if ( is_control_page_mapped )
		Memory::Unmap(control_alloc.from);
	FreeKernelAddress(&control_alloc);
	if ( is_dma_page_mapped )
		Memory::Unmap(dma_alloc.from);
	FreeKernelAddress(&dma_alloc);
	if ( control_physical_frame )
		Page::Put(control_physical_frame, PAGE_USAGE_DRIVER);
	if ( dma_physical_frame )
		Page::Put(dma_physical_frame, PAGE_USAGE_DRIVER);
}

void Port::LogF(const char* format, ...)
{
	// TODO: Print this line in an atomic manner.
	Log::PrintF("ahci: pci 0x%X: port %u: ", hba->devaddr, port_index);
	va_list ap;
	va_start(ap, format);
	Log::PrintFV(format, ap);
	va_end(ap);
	Log::PrintF("\n");
}

bool Port::Initialize()
{
	// TODO: Potentially move the wait-for-device-to-be-idle code here from hba.

	// Clear interrupt status.
	regs->pxis = regs->pxis;

	// Clear error bits.
	regs->pxserr = regs->pxserr;

	if ( !(control_physical_frame = Page::Get(PAGE_USAGE_DRIVER)) )
	{
		LogF("error: control page allocation failure");
		return false;
	}

	if ( !(dma_physical_frame = Page::Get(PAGE_USAGE_DRIVER)) )
	{
		LogF("error: dma page allocation failure");
		return false;
	}

	if ( !AllocateKernelAddress(&control_alloc, Page::Size()) )
	{
		LogF("error: control page virtual address allocation failure");
		return false;
	}

	if ( !AllocateKernelAddress(&dma_alloc, Page::Size()) )
	{
		LogF("error: dma page virtual address allocation failure");
		return false;
	}

	int prot = PROT_KREAD | PROT_KWRITE;
	is_control_page_mapped =
		Memory::Map(control_physical_frame, control_alloc.from, prot);
	if ( !is_control_page_mapped )
	{
		LogF("error: control page virtual address allocation failure");
		return false;
	}

	Memory::Flush();

	is_dma_page_mapped = Memory::Map(dma_physical_frame, dma_alloc.from, prot);
	if ( !is_dma_page_mapped )
	{

		LogF("dma page virtual address allocation failure");
		return false;
	}

	Memory::Flush();

	return true;
}

bool Port::FinishInitialize()
{
	// Disable power management transitions for now (IPM = 3 = transitions to
	// partial/slumber disabled).
	regs->pxsctl = regs->pxsctl | 0x300 /* TODO: Magic constant? */;

	// Power on and spin up the device if necessary.
	if ( regs->pxcmd & PXCMD_CPD )
		regs->pxcmd = regs->pxcmd | PXCMD_POD;
	if ( hba->regs->cap & CAP_SSS )
		regs->pxcmd = regs->pxcmd | PXCMD_SUD;

	// Activate the port.
	regs->pxcmd = (regs->pxcmd & ~PXCMD_ICC(16)) | (1 << 28);

	if ( !Reset() )
	{
		// TODO: Is this safe?
		return false;
	}

	// Clear interrupt status.
	regs->pxis = regs->pxis;

	// Clear error bits.
	regs->pxserr = regs->pxserr;

	uintptr_t virt = control_alloc.from;
	uintptr_t phys = control_physical_frame;

	memset((void*) virt, 0, Page::Size());

	size_t offset = 0;

	clist = (volatile struct command_header*) (virt + offset);
	uint64_t pxclb_addr = phys + offset;
	regs->pxclb = pxclb_addr >> 0;
	regs->pxclbu = pxclb_addr >> 32;
	offset += sizeof(struct command_header) * AHCI_COMMAND_HEADER_COUNT;

	fis = (volatile struct fis*) (virt + offset);
	uint64_t pxf_addr = phys + offset;
	regs->pxfb = pxf_addr >> 0;
	regs->pxfbu = pxf_addr >> 32;
	offset += sizeof(struct fis);

	ctbl = (volatile struct command_table*) (virt + offset);
	uint64_t ctba_addr = phys + offset;
	clist[0].ctba = ctba_addr >> 0;
	clist[0].ctbau = ctba_addr >> 32;
	offset += sizeof(struct command_table);

	prdt = (volatile struct prd*) (virt + offset);
	offset += sizeof(struct prd);
	// TODO: There can be more of these, fill until end of page!

	// Enable FIS receive.
	regs->pxcmd = regs->pxcmd | PXCMD_FRE;
	ahci_port_flush(regs);

	assert(offset <= Page::Size());

	uint32_t ssts = regs->pxssts;
	uint32_t pxtfd = regs->pxtfd;

	if ( (ssts & 0xF) != 0x3 )
		return false;
	if ( pxtfd & ATA_STATUS_BSY )
		return false;
	if ( pxtfd & ATA_STATUS_DRQ )
		return false;

	// TODO: ATAPI.
	if ( regs->pxsig == 0xEB140101 )
		return false;

	// Start DMA engine.
	if ( regs->pxcmd & PXCMD_CR )
	{
		if ( !WaitClear(&regs->pxcmd, PXCMD_CR, false, 500) )
		{
			LogF("error: timeout waiting for PXCMD_CR to clear");
			return false;
		}
	}
	regs->pxcmd = regs->pxcmd | PXCMD_ST;
	ahci_port_flush(regs);

	// Set which interrupts we want to know about.
	regs->pxie = PORT_INTR_ERROR | PXIE_DHRE | PXIE_PSE |
	             PXIE_DSE | PXIE_SDBE | PXIE_DPE;
	ahci_port_flush(regs);

	CommandDMA(ATA_CMD_IDENTIFY, 512, false);
	if ( !AwaitInterrupt(500 /*ms*/) )
	{
		LogF("error: IDENTIFY timed out");
		return false;
	}
	transfer_in_progress = false;

	memcpy(identify_data, (void*) dma_alloc.from, sizeof(identify_data));

	little_uint16_t* words = (little_uint16_t*) dma_alloc.from;

	if ( words[0] & (1 << 15) )
		return errno = EINVAL, false; // Skipping non-ATA device.
	if ( !(words[49] & (1 << 9)) )
		return errno = EINVAL, false; // Skipping non-LBA device.

	this->is_lba48 = words[83] & (1 << 10);

	copy_ata_string(serial, (const char*) &words[10], sizeof(serial) - 1);
	copy_ata_string(revision, (const char*) &words[23], sizeof(revision) - 1);
	copy_ata_string(model, (const char*) &words[27], sizeof(model) - 1);

	uint64_t block_count;
	if ( is_lba48 )
	{
		block_count = (uint64_t) words[100] << 0 |
		              (uint64_t) words[101] << 16 |
		              (uint64_t) words[102] << 32 |
		              (uint64_t) words[103] << 48;
	}
	else
	{
		block_count = (uint64_t) words[60] << 0 |
		              (uint64_t) words[61] << 16;
	}

	uint64_t block_size = 512;
	if(  (words[106] & (1 << 14)) &&
	    !(words[106] & (1 << 15)) &&
	     (words[106] & (1 << 12)) )
	{
		block_size = 2 * ((uint64_t) words[117] << 0 |
		                  (uint64_t) words[118] << 16);
	}

	// TODO: Verify the block size is a power of two.

	cylinder_count = words[1];
	head_count = words[3];
	sector_count = words[6];

	// TODO: Verify that DMA is properly supported.
	//       See kiwi/source/drivers/bus/ata/device.c line 344.

	if ( __builtin_mul_overflow(block_count, block_size, &this->device_size) )
	{
		LogF("error: device size overflows off_t");
		return errno = EOVERFLOW, false;
	}

	this->block_count = (blkcnt_t) block_count;
	this->block_size = (blkcnt_t) block_size;

	return true;
}

bool Port::Reset()
{
	if ( regs->pxcmd & (PXCMD_ST | PXCMD_CR) )
	{
		regs->pxcmd = regs->pxcmd & ~PXCMD_ST;
		if ( !WaitClear(&regs->pxcmd, PXCMD_CR, false, 500) )
		{
			LogF("error: timeout waiting for PXCMD_CR to clear");
			return false;
		}
	}

	if ( regs->pxcmd & (PXCMD_FRE | PXCMD_FR) )
	{
		regs->pxcmd = regs->pxcmd & ~PXCMD_FRE;
		if ( !WaitClear(&regs->pxcmd, PXCMD_FR, false, 500) )
		{
			LogF("error: timeout waiting for PXCMD_FR to clear");
			return false;
		}
	}

#if 0
	// Reset the device
	regs->pxsctl = (regs->pxsctl & ~0xF) | 1;
	ahci_port_flush(regs);
	delay(1500);
	regs->pxsctl = (regs->pxsctl & ~0xF);
	ahci_port_flush(regs);
#endif

	// Wait for the device to be detected.
	if ( !WaitSet(&regs->pxssts, 0x1, false, 600) )
		return false;

	// Clear error.
	regs->pxserr = regs->pxserr;
	ahci_port_flush(regs);

	// Wait for communication to be established with device
	if ( regs->pxssts & 0x1 )
	{
		if ( !WaitSet(&regs->pxssts, 0x3, false, 600) )
			return false;
		regs->pxserr = regs->pxserr;
		ahci_port_flush(regs);
	}

	// Wait for the device to come back up.
	if ( (regs->pxtfd & 0xFF) == 0xFF )
	{
		delay(500 * 1000);
		if ( (regs->pxtfd & 0xFF) == 0xFF )
		{
			LogF("error: device did not come back up after reset");
			return errno = EINVAL, false;
		}
	}

#if 0
	if ( !WaitClear(&regs->pxtfd, ATA_STATUS_BSY, false, 1000) )
	{
		LogF("error: device did not unbusy");
		return false;
	}
#endif

	return true;
}

void Port::Seek(blkcnt_t block_index, size_t count)
{
	uintmax_t lba = (uintmax_t) block_index;
	if ( is_lba48 )
	{
		memset((void *)&ctbl->cfis, 0, sizeof(ctbl->cfis));
		ctbl->cfis.count_0_7  = (count >> 0) & 0xff;
		ctbl->cfis.count_8_15 = (count >> 8) & 0xff;
		ctbl->cfis.lba_0_7    = (lba >> 0) & 0xff;
		ctbl->cfis.lba_8_15   = (lba >> 8) & 0xff;
		ctbl->cfis.lba_16_23  = (lba >> 16) & 0xff;
		ctbl->cfis.lba_24_31  = (lba >> 24) & 0xff;
		ctbl->cfis.lba_32_39  = (lba >> 32) & 0xff;
		ctbl->cfis.lba_40_47  = (lba >> 40) & 0xff;
		ctbl->cfis.device = 0x40;
	}
	else
	{
		ctbl->cfis.count_0_7  = (count >> 0) & 0xff;
		ctbl->cfis.lba_0_7    = (lba >> 0) & 0xff;
		ctbl->cfis.lba_8_15   = (lba >> 8) & 0xff;
		ctbl->cfis.lba_16_23  = (lba >> 16) & 0xff;
		ctbl->cfis.device = 0x40 | ((lba >> 24) & 0xf);
	}
}

void Port::CommandDMA(uint8_t cmd, size_t size, bool write)
{
	if ( 0 < size )
	{
		assert(size <= Page::Size());
		assert((size & 1) == 0); /* sizes & addresses must be 2-byte aligned */
	}

	// Set up the command header.
	uint16_t fis_length = 5 /* dwords */;
	uint16_t prdtl = size ? 1 : 0;
	uint16_t clist_0_dw0l = fis_length;
	if ( write )
		clist_0_dw0l |= COMMAND_HEADER_DW0_WRITE;
	clist[0].dw0l = clist_0_dw0l;
	clist[0].prdtl = prdtl;
	clist[0].prdbc = 0;

	// Set up the physical region descriptor.
	if ( prdtl )
	{
		uint32_t prdt_0_dbc = size - 1;
		uint32_t prdt_0_dw3 = prdt_0_dbc;
		prdt[0].dba  = (uint64_t) dma_physical_frame >>  0 & 0xFFFFFFFF;
		prdt[0].dbau = (uint64_t) dma_physical_frame >> 32 & 0xFFFFFFFF;
		prdt[0].reserved1 = 0;
		prdt[0].dw3 = prdt_0_dw3;
	}

	// Set up the command table.
	ctbl->cfis.type = 0x27;
	ctbl->cfis.pm_port = 0;
	ctbl->cfis.c_bit = 1;
	ctbl->cfis.command = cmd;

	// Anticipate we will be delivered an IRQ.
	PrepareAwaitInterrupt();
	transfer_in_progress = true;
	transfer_size = size;
	transfer_is_write = write;

	// Execute the command.
	regs->pxci = 1;
	ahci_port_flush(regs);
}

bool Port::FinishTransferDMA()
{
	assert(transfer_in_progress);

	// Wait for an interrupt to arrive.
	if ( !AwaitInterrupt(10000 /*ms*/) )
	{
		const char* op = transfer_is_write ? "write" : "read";
		LogF("error: %s timed out", op);
		transfer_in_progress = false;
		return errno = EIO, false;
	}

	while ( transfer_is_write && regs->pxtfd & ATA_STATUS_BSY )
		kthread_yield();

	transfer_in_progress = false;
	return true;
}

off_t Port::GetSize()
{
	return device_size;
}

blkcnt_t Port::GetBlockCount()
{
	return block_count;
}

blksize_t Port::GetBlockSize()
{
	return block_size;
}

uint16_t Port::GetCylinderCount()
{
	return cylinder_count;
}

uint16_t Port::GetHeadCount()
{
	return head_count;
}

uint16_t Port::GetSectorCount()
{
	return sector_count;
}

const char* Port::GetDriver()
{
	return "ahci";
}

const char* Port::GetModel()
{
	return model;
}

const char* Port::GetSerial()
{
	return serial;
}

const char* Port::GetRevision()
{
	return revision;
}

const unsigned char* Port::GetATAIdentify(size_t* size_ptr)
{
	return *size_ptr = sizeof(identify_data), identify_data;
}

int Port::sync(ioctx_t* ctx)
{
	(void) ctx;
	ScopedLock lock(&port_lock);
	if ( transfer_in_progress && !FinishTransferDMA() )
		return -1;
	PrepareAwaitInterrupt();
	uint8_t cmd = is_lba48 ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;
	CommandDMA(cmd, 0, false);
	// TODO: This might take longer than 30 seconds according to the spec. But
	//       how long? Let's say twice that?
	if ( !AwaitInterrupt(2 * 30000 /*ms*/) )
	{
		LogF("error: cache flush timed out");
		transfer_in_progress = false;
		return errno = EIO, -1;
	}
	transfer_in_progress = false;
	if ( regs->pxtfd & (ATA_STATUS_ERR | ATA_STATUS_DF) )
	{
		LogF("error: IO error");
		return errno = EIO, -1;
	}
	return 0;
}

ssize_t Port::pread(ioctx_t* ctx, unsigned char* buf, size_t count, off_t off)
{
	ScopedLock lock(&port_lock);
	ssize_t result = 0;
	while ( count )
	{
		if ( device_size <= off )
			break;
		if ( (uintmax_t) device_size - off < (uintmax_t) count )
			count = (size_t) device_size - off;
		uintmax_t block_index = (uintmax_t) off / (uintmax_t) block_size;
		uintmax_t block_offset = (uintmax_t) off % (uintmax_t) block_size;
		uintmax_t amount = block_offset + count;
		if ( Page::Size() < amount )
			amount = Page::Size();
		size_t num_blocks = (amount + block_size - 1) / block_size;
		uintmax_t full_amount = num_blocks * block_size;
		// If an asynchronous operation is in progress, let it finish.
		if ( transfer_in_progress && !FinishTransferDMA() )
			return result ? result : -1;
		unsigned char* dma_data = (unsigned char*) dma_alloc.from;
		unsigned char* data = dma_data + block_offset;
		size_t data_size = amount - block_offset;
		Seek(block_index, num_blocks);
		uint8_t cmd = is_lba48 ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_DMA;
		CommandDMA(cmd, (size_t) full_amount, false);
		if ( !FinishTransferDMA() )
			return result ? result : -1;
		if ( !ctx->copy_to_dest(buf, data, data_size) )
			return result ? result : -1;
		buf += data_size;
		count -= data_size;
		result += data_size;
		off += data_size;
	}
	return result;
}

ssize_t Port::pwrite(ioctx_t* ctx, const unsigned char* buf, size_t count, off_t off)
{
	ScopedLock lock(&port_lock);
	ssize_t result = 0;
	while ( count )
	{
		if ( device_size <= off )
			break;
		if ( (uintmax_t) device_size - off < (uintmax_t) count )
			count = (size_t) device_size - off;
		uintmax_t block_index = (uintmax_t) off / (uintmax_t) block_size;
		uintmax_t block_offset = (uintmax_t) off % (uintmax_t) block_size;
		uintmax_t amount = block_offset + count;
		if ( Page::Size() < amount )
			amount = Page::Size();
		size_t num_blocks = (amount + block_size - 1) / block_size;
		uintmax_t full_amount =  num_blocks * block_size;
		// If an asynchronous operation is in progress, let it finish.
		if ( transfer_in_progress && !FinishTransferDMA() )
			return result ? result : -1;
		unsigned char* dma_data = (unsigned char*) dma_alloc.from;
		unsigned char* data = dma_data + block_offset;
		size_t data_size = amount - block_offset;
		if ( block_offset || amount < full_amount )
		{
			Seek(block_index, num_blocks);
			uint8_t cmd = is_lba48 ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_DMA;
			CommandDMA(cmd, (size_t) full_amount, false);
			if ( !FinishTransferDMA() )
				return result ? result : -1;
		}
		if ( !ctx->copy_from_src(data, buf, data_size) )
			return result ? result : -1;
		Seek(block_index, num_blocks);
		uint8_t cmd = is_lba48 ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_WRITE_DMA;
		CommandDMA(cmd, (size_t) full_amount, true);
		// Let the transfer finish asynchronously so the caller can prepare the
		// next write operation to keep the write pipeline busy.
		buf += data_size;
		count -= data_size;
		result += data_size;
		off += data_size;
	}
	return result;
}

void Port::PrepareAwaitInterrupt()
{
	interrupt_signaled = false;
}

bool Port::AwaitInterrupt(unsigned int msecs)
{
	struct timespec timeout = timespec_make(msecs / 1000, (msecs % 1000) * 1000000L);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	struct timespec begun;
	clock->Get(&begun, NULL);
	while ( true )
	{
		struct timespec now;
		clock->Get(&now, NULL);
		if ( interrupt_signaled )
			return true;
		struct timespec elapsed = timespec_sub(now, begun);
		if ( timespec_le(timeout, elapsed) )
			return errno = ETIMEDOUT, false;
		// TODO: Can't safely back out here unless the pending operation is
		//       is properly cancelled.
		//if ( Signal::IsPending() )
		//	return errno = EINTR, false;
		kthread_yield();
	}
}

void Port::OnInterrupt()
{
	// Check whether any interrupt are pending.
	uint32_t is = regs->pxis;
	if ( !is )
		return;

	// Clear the pending interrupts.
	regs->pxis = is;

	// Handle error interrupts.
	if ( is & PORT_INTR_ERROR )
	{
		regs->pxserr = regs->pxserr;
		// TODO: How exactly should this be handled?
	}

	if ( !interrupt_signaled )
	{
		interrupt_signaled = true;
		// TODO: Priority schedule the blocking thread now.
	}
}

} // namespace AHCI
} // namespace Sortix
