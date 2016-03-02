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
 * disk/ata/port.h
 * Driver for ATA.
 */

#ifndef SORTIX_DISK_ATA_PORT_H
#define SORTIX_DISK_ATA_PORT_H

#include <sys/types.h>

#include <stdint.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/harddisk.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kthread.h>

namespace Sortix {
namespace ATA {

class Channel;

class Port : public Harddisk
{
	friend class Channel;

public:
	Port(Channel* channel, unsigned int port_index);
	virtual ~Port();

public:
	virtual off_t GetSize();
	virtual blkcnt_t GetBlockCount();
	virtual blksize_t GetBlockSize();
	virtual uint16_t GetCylinderCount();
	virtual uint16_t GetHeadCount();
	virtual uint16_t GetSectorCount();
	virtual const char* GetDriver();
	virtual const char* GetModel();
	virtual const char* GetSerial();
	virtual const char* GetRevision();
	virtual const unsigned char* GetATAIdentify(size_t* size_ptr);
	virtual int sync(ioctx_t* ctx);
	virtual ssize_t pread(ioctx_t* ctx, unsigned char* buf, size_t count, off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const unsigned char* buf, size_t count, off_t off);

public:
	bool Initialize();
	bool FinishInitialize();

private:
	__attribute__((format(printf, 2, 3)))
	void LogF(const char* format, ...);
	void Seek(blkcnt_t block_index, size_t count);
	void CommandDMA(uint8_t cmd, size_t size, bool write);
	void CommandPIO(uint8_t cmd, size_t size, bool write);
	bool FinishTransferDMA();
	bool TransferPIO(size_t size, bool write);
	void PrepareAwaitInterrupt();
	bool AwaitInterrupt(unsigned int msescs);
	void OnInterrupt();

private:
	unsigned char identify_data[512];
	char serial[20 + 1];
	char revision[8 + 1];
	char model[40 + 1];
	addralloc_t control_alloc;
	addralloc_t dma_alloc;
	Channel* channel;
	volatile struct prd* prdt;
	addr_t control_physical_frame;
	addr_t dma_physical_frame;
	unsigned int port_index;
	bool is_control_page_mapped;
	bool is_dma_page_mapped;
	bool is_lba48;
	bool is_using_dma;
	off_t device_size;
	blksize_t block_count;
	blkcnt_t block_size;
	uint16_t cylinder_count;
	uint16_t head_count;
	uint16_t sector_count;
	volatile bool interrupt_signaled;
	bool transfer_in_progress;
	size_t transfer_size;
	bool transfer_is_write;

};

} // namespace ATA
} // namespace Sortix

#endif
