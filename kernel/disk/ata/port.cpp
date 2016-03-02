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
 * disk/ata/port.cpp
 * Driver for ATA.
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <endian.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/mman.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/clock.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/time.h>

#include "hba.h"
#include "port.h"
#include "registers.h"

namespace Sortix {
namespace ATA {

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

static void sleep_400_nanoseconds()
{
	struct timespec delay = timespec_make(0, 400);
	Clock* clock = Time::GetClock(CLOCK_BOOT);
	clock->SleepDelay(delay);
}

Port::Port(Channel* channel, unsigned int port_index)
{
	this->channel = channel;
	this->port_index = port_index;
	memset(&control_alloc, 0, sizeof(control_alloc));
	memset(&dma_alloc, 0, sizeof(dma_alloc));
	is_control_page_mapped = false;
	is_dma_page_mapped = false;
	interrupt_signaled = false;
	transfer_in_progress = false;
	control_physical_frame = 0;
	dma_physical_frame = 0;
}

Port::~Port()
{
	if ( transfer_in_progress )
		FinishTransferDMA();
	if ( is_control_page_mapped )
	{
		Memory::Unmap(control_alloc.from);
		Memory::Flush();
	}
	if ( is_dma_page_mapped )
	{
		Memory::Unmap(dma_alloc.from);
		Memory::Flush();
	}
	FreeKernelAddress(&control_alloc);
	FreeKernelAddress(&dma_alloc);
	if ( control_physical_frame )
		Page::Put(control_physical_frame, PAGE_USAGE_DRIVER);
	if ( dma_physical_frame )
		Page::Put(dma_physical_frame, PAGE_USAGE_DRIVER);
}

void Port::LogF(const char* format, ...)
{
	// TODO: Print this line in an atomic manner.
	const char* cdesc = channel->channel_index == 0 ? "primary" : "secondary";
	const char* ddesc = port_index == 0 ? "master" : "slave";
	Log::PrintF("ata: pci 0x%X: %s %s: ", channel->devaddr, cdesc, ddesc);
	va_list ap;
	va_start(ap, format);
	Log::PrintFV(format, ap);
	va_end(ap);
	Log::PrintF("\n");
}

bool Port::Initialize()
{
	if ( !(control_physical_frame = Page::Get32Bit(PAGE_USAGE_DRIVER)) )
	{
		LogF("error: control page allocation failure");
		return false;
	}

	if ( !(dma_physical_frame = Page::Get32Bit(PAGE_USAGE_DRIVER)) )
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
		LogF("error: dma page virtual address allocation failure");
		return false;
	}

	Memory::Flush();

	prdt = (volatile struct prd*) (control_alloc.from);

	return true;
}

bool Port::FinishInitialize()
{
	ScopedLock lock(&channel->hw_lock);

	channel->SelectDrive(port_index);

	outport8(channel->port_base + REG_COMMAND, CMD_IDENTIFY);

	sleep_400_nanoseconds();

	// TODO: The status polling logic should be double-checked against some
	//       formal specification telling how this should properly be done.

	uint8_t status = inport8(channel->port_base + REG_STATUS);
	if ( status == 0 )
	{
		// Non-existent.
		return errno = ENODEV, false;
	}

	// TODO: This failing might mean non-existent.
	// TODO: What is a good timeout here?
	if ( !wait_inport8_clear(channel->port_base + REG_STATUS, STATUS_BUSY,
	                         false, 1000 /*ms*/) )
	{
		// IDENTIFY timed out, still busy.
		return errno = ETIMEDOUT, false;
	}

	// TODO: This failing might mean non-existent.
	// TODO: Should we actually wait here, or are the status set already?
	// TODO: What is a good timeout here?
	if ( !wait_inport8_set(channel->port_base + REG_STATUS,
	                       STATUS_DATAREADY | STATUS_ERROR, true, 1000 /*ms*/) )
	{
		// IDENTIFY timed out, status not set.
		return errno = ETIMEDOUT, false;
	}

	status = inport8(channel->port_base + REG_STATUS);
	if ( status & STATUS_ERROR )
	{
		uint8_t mid = inport8(channel->port_base + REG_LBA_MID);
		uint8_t high = inport8(channel->port_base + REG_LBA_HIGH);

		if ( (mid == 0x14 && high == 0xEB) || (mid == 0x69 && high == 0x96) )
		{
			// TODO: Add ATAPI support.
			//LogF("ignoring: found ATAPI device instead");
			return errno = ENODRV, false;
		}
		else if ( mid == 0x3C && high == 0xC3 )
		{
			// TODO: Does this actually happen and can we do something?
			//LogF("ignoring: found SATA device instead");
			return errno = ENODRV, false;
		}
		else if ( (mid || high) && (mid != 0x7F && high != 0x7F) )
		{
			//LogF("ignoring: found unknown device instead (0x%02X:0x%02X)",
			//     mid, high);
			return errno = ENODRV, false;
		}
		else if ( mid == 0x7F && high == 0x7F )
		{
			//LogF("ignoring: non-existent");
			return errno = EIO, false;
		}
		else
		{
			LogF("ignoring: IDENTIFY returned error status");
			return errno = EIO, false;
		}
	}

	for ( size_t i = 0; i < 256; i++ )
	{
		uint16_t value = inport16(channel->port_base + REG_DATA);
		identify_data[2*i + 0] = value >> 0 & 0xFF;
		identify_data[2*i + 1] = value >> 8 & 0xFF;
	}

	little_uint16_t* words = (little_uint16_t*) identify_data;

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

	is_using_dma = true;

	// This is apparently the case on older hardware, there are additional
	// restrictions on DMA usage, we could work around it, but instead we just
	// don't use DMA at all, as such hardware should be irrelevant by now. Print
	// a warning so we can look into this if it actually turns out to matter.
	// TODO: Arg. This happens in VirtualBox!
	// TODO: Alternative solution. If this is true, then it should lock the
	//       HBA hw_lock instead than the channel one.
	uint8_t bm_status = inport8(channel->busmaster_base + BUSMASTER_REG_STATUS);
	if ( bm_status & BUSMASTER_STATUS_SIMPLEX )
	{
		LogF("warning: simplex silliness: no DMA");
		is_using_dma = false;
	}

	// TODO: Why is this commented out? Due to the above?
#if 0
	if ( port_index == 0 && !(bm_status & BUSMASTER_STATUS_MASTER_DMA_INIT) )
	{
		LogF("warning: no DMA support");
		is_using_dma = false;
	}

	if ( port_index == 1 && !(bm_status & BUSMASTER_STATUS_SLAVE_DMA_INIT) )
	{
		LogF("warning: no DMA support");
		is_using_dma = false;
	}
#endif

	if ( __builtin_mul_overflow(block_count, block_size, &this->device_size) )
	{
		LogF("error: device size overflows off_t");
		return errno = EOVERFLOW, false;
	}

	this->block_count = (blkcnt_t) block_count;
	this->block_size = (blkcnt_t) block_size;

	return true;
}

void Port::Seek(blkcnt_t block_index, size_t count)
{
	uintmax_t lba = (uintmax_t) block_index;
	uint8_t mode = (is_lba48 ? 0x40 : 0xE0) | port_index << 4;
	outport8(channel->port_base + REG_DRIVE_SELECT, mode);
	if ( is_lba48 )
	{
		outport8(channel->port_base + REG_SECTOR_COUNT, count >> 8 & 0xFF);
		outport8(channel->port_base + REG_LBA_LOW, lba >> 24 & 0xFF);
		outport8(channel->port_base + REG_LBA_MID, lba >> 32 & 0xFF);
		outport8(channel->port_base + REG_LBA_HIGH, lba >> 40 & 0xFF);
	}
	outport8(channel->port_base + REG_SECTOR_COUNT, count >> 0 & 0xFF);
	outport8(channel->port_base + REG_LBA_LOW, lba >> 0 & 0xFF);
	outport8(channel->port_base + REG_LBA_MID, lba >> 8 & 0xFF);
	outport8(channel->port_base + REG_LBA_HIGH, lba >> 16 & 0xFF);
}

void Port::CommandDMA(uint8_t cmd, size_t size, bool write)
{
	assert(size);
	assert(size <= Page::Size());
	assert(size <= UINT16_MAX);
	assert((size & 1) == 0); /* sizes and addresses must be 2-byte aligned */

	// Store the DMA region in the first PRD.
	prdt->physical = dma_physical_frame >> 0 & 0xFFFFFFFF;
	prdt->count = size;
	prdt->flags = PRD_FLAG_EOT;

	// Tell the hardware the location of the PRDT.
	uint32_t bm_prdt = control_physical_frame >> 0 & 0xFFFFFFFF;
	outport32(channel->busmaster_base + BUSMASTER_REG_PDRT, bm_prdt);

	// Clear the error and interrupt bits.
	uint8_t bm_status = inport8(channel->busmaster_base + BUSMASTER_REG_STATUS);
	bm_status |= BUSMASTER_STATUS_DMA_FAILURE | BUSMASTER_STATUS_INTERRUPT_PENDING;
	outport8(channel->busmaster_base + BUSMASTER_REG_STATUS, bm_status);

	// Set the transfer direction
	uint8_t bm_command = inport8(channel->busmaster_base + BUSMASTER_REG_COMMAND);
	if ( write )
		bm_command &= ~BUSMASTER_COMMAND_READING;
	else
		bm_command |= BUSMASTER_COMMAND_READING;
	outport8(channel->busmaster_base + BUSMASTER_REG_COMMAND, bm_command);

	// Anticipate we will be delivered an IRQ.
	PrepareAwaitInterrupt();
	transfer_in_progress = true;
	transfer_size = size;
	transfer_is_write = write;

	// Execute the command.
	outport8(channel->port_base + REG_COMMAND, cmd);

	// Start the DMA transfer.
	bm_command = inport8(channel->busmaster_base + BUSMASTER_REG_COMMAND);
	bm_command |= BUSMASTER_COMMAND_START;
	outport8(channel->busmaster_base + BUSMASTER_REG_COMMAND, bm_command);
}

bool Port::FinishTransferDMA()
{
	assert(transfer_in_progress);

	bool result = true;

	// Wait for an interrupt to arrive.
	if ( !AwaitInterrupt(10000 /*ms*/) )
	{
		const char* op = transfer_is_write ? "write" : "read";
		LogF("error: %s DMA timed out", op);
		errno = EIO;
		result = false;
		// TODO: Is this a consistent state, do we need a reset, how to recover?
	}

	// Stop the DMA transfer.
	uint8_t bm_command = inport8(channel->busmaster_base + BUSMASTER_REG_COMMAND);
	bm_command &= ~BUSMASTER_COMMAND_START;
	outport8(channel->busmaster_base + BUSMASTER_REG_COMMAND, bm_command);

	// Clear the error and interrupt bits.
	uint8_t bm_status = inport8(channel->busmaster_base + BUSMASTER_REG_STATUS);
	uint8_t bm_set_flags = BUSMASTER_STATUS_DMA_FAILURE |
	                       BUSMASTER_STATUS_INTERRUPT_PENDING;
	outport8(channel->busmaster_base + BUSMASTER_REG_STATUS,
	         bm_status | bm_set_flags);

	// Check if the DMA transfer failed.
	if ( bm_status & BUSMASTER_STATUS_DMA_FAILURE )
	{
		const char* op = transfer_is_write ? "write" : "read";
		LogF("error: %s DMA error", op);
		errno = EIO;
		result = false;
		// TODO: Is this a consistent state, do we need a reset, how to recover?
	}

	transfer_in_progress = false;
	return result;
}

void Port::CommandPIO(uint8_t cmd, size_t size, bool write)
{
	assert(size);
	assert(size <= Page::Size());
	assert((size & 1) == 0); /* sizes and addresses must be 2-byte aligned */

	// Anticipate we will be delivered an IRQ.
	PrepareAwaitInterrupt();

	// Execute the command.
	(void) write;
	outport8(channel->port_base + REG_COMMAND, cmd);
}

bool Port::TransferPIO(size_t size, bool write)
{
	const char* op = write ? "write" : "read";
	size_t i = 0;
	while ( write || true )
	{
		if ( !write && i < size && !AwaitInterrupt(10000 /*ms*/) )
		{
			LogF("error: %s timed out, waiting for transfer start", op);
			return errno = EIO, false;
		}

		if ( !wait_inport8_clear(channel->port_base + REG_STATUS,
		                         STATUS_BUSY, false, 10000 /*ms*/) )
		{
			LogF("error: %s timed out waiting for transfer pre idle", op);
			return errno = EIO, false;
		}

		uint8_t status = inport8(channel->port_base + REG_STATUS);

		if ( status & STATUS_BUSY )
		{
			LogF("error: %s unexpectedly still busy", op);
			return errno = EIO, false;
		}

		if ( status & (STATUS_ERROR | STATUS_DRIVEFAULT) )
		{
			LogF("error: %s error", op);
			return errno = EIO, false;
		}

		if ( i == size )
			break;

		if ( !(status & STATUS_DATAREADY) )
		{
			LogF("error: %s unexpectedly not ready", op);
			return errno = EIO, false;
		}

		// Anticipate another IRQ if we're not at the end.
		size_t i_sector_end = i + block_size;
		if ( i_sector_end != size )
			PrepareAwaitInterrupt();

		uint8_t* dma_data = (uint8_t*) dma_alloc.from;

		if ( write )
		{
			while ( i < size && i < i_sector_end )
			{
				uint16_t value = dma_data[i + 0] << 0 | dma_data[i + 1] << 8;
				outport16(channel->port_base + REG_DATA, value);
				i += 2;
			}
		}
		else
		{
			while ( i < size && i < i_sector_end )
			{
				uint16_t value = inport16(channel->port_base + REG_DATA);
				dma_data[i + 0] = (value >> 0) & 0xFF;
				dma_data[i + 1] = (value >> 8) & 0xFF;
				i += 2;
			}
		}

		if ( write && !AwaitInterrupt(10000 /*ms*/) )
		{
			LogF("error: %s timed out, waiting for transfer end", op);
			return errno = EIO, false;
		}
	}

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
	return "ata";
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
	ScopedLock lock(&channel->hw_lock);
	channel->SelectDrive(port_index);
	if ( transfer_in_progress && !FinishTransferDMA() )
		return -1;
	PrepareAwaitInterrupt();
	uint8_t cmd = is_lba48 ? CMD_FLUSH_CACHE_EXT : CMD_FLUSH_CACHE;
	outport8(channel->port_base + REG_COMMAND, cmd);
	// TODO: This might take longer than 30 seconds according to the spec. But
	//       how long? Let's say twice that?
	if ( !AwaitInterrupt(2 * 30000 /*ms*/) )
	{
		LogF("error: cache flush timed out");
		transfer_in_progress = false;
		return errno = EIO, -1;
	}
	transfer_in_progress = false;
	uint8_t status = inport8(channel->port_base + REG_STATUS);
	if ( status & (STATUS_ERROR | STATUS_DRIVEFAULT) )
	{
		LogF("error: IO error");
		return errno = EIO, -1;
	}
	return 0;
}

ssize_t Port::pread(ioctx_t* ctx, unsigned char* buf, size_t count, off_t off)
{
	ssize_t result = 0;
	while ( count )
	{
		ScopedLock lock(&channel->hw_lock);
		channel->SelectDrive(port_index);
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
		if ( is_using_dma )
		{
			uint8_t cmd = is_lba48 ? CMD_READ_DMA_EXT : CMD_READ_DMA;
			CommandDMA(cmd, (size_t) full_amount, false);
			if ( !FinishTransferDMA() )
				return result ? result : -1;
		}
		else
		{
			uint8_t cmd = is_lba48 ? CMD_READ_EXT : CMD_READ;
			CommandPIO(cmd, (size_t) full_amount, false);
			if ( !TransferPIO((size_t) full_amount, false) )
				return result ? result : -1;
		}
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
	ssize_t result = 0;
	while ( count )
	{
		ScopedLock lock(&channel->hw_lock);
		channel->SelectDrive(port_index);
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
		if ( block_offset || amount < full_amount )
		{
			if ( is_using_dma )
			{
				uint8_t cmd = is_lba48 ? CMD_READ_DMA_EXT : CMD_READ_DMA;
				Seek(block_index, num_blocks);
				CommandDMA(cmd, (size_t) full_amount, false);
				if ( !FinishTransferDMA() )
					return result ? result : -1;
			}
			else
			{
				uint8_t cmd = is_lba48 ? CMD_READ_EXT : CMD_READ;
				Seek(block_index, num_blocks);
				CommandPIO(cmd, (size_t) full_amount, false);
				if ( !TransferPIO((size_t) full_amount, false) )
					return result ? result : -1;
			}
		}
		if ( !ctx->copy_from_src(data, buf, data_size) )
			return result ? result : -1;
		if ( is_using_dma )
		{
			Seek(block_index, num_blocks);
			uint8_t cmd = is_lba48 ? CMD_WRITE_DMA_EXT : CMD_WRITE_DMA;
			CommandDMA(cmd, (size_t) full_amount, true);
			// Let the transfer finish asynchronously so the caller can prepare
			// the next write operation to keep the write pipeline busy.
		}
		else
		{
			Seek(block_index, num_blocks);
			uint8_t cmd = is_lba48 ? CMD_WRITE_EXT : CMD_WRITE;
			CommandPIO(cmd, (size_t) full_amount, true);
			if ( !TransferPIO((size_t) full_amount, true) )
				return result ? result : -1;
		}
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
	// Clear INTRQ.
	uint8_t status = inport8(channel->port_base + REG_STATUS);

	if ( status & STATUS_ERROR )
	{
		uint8_t error_status = inport8(channel->port_base + REG_ERROR);
		(void) error_status; // TODO: How to handle this exactly?
	}

	if ( !interrupt_signaled )
	{
		interrupt_signaled = true;
		// TODO: Priority schedule the blocking thread now.
	}
}

} // namespace ATA
} // namespace Sortix
