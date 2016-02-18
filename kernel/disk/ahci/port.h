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

    disk/ahci/port.h
    Driver for the Advanced Host Controller Interface.

*******************************************************************************/

#ifndef SORTIX_DISK_AHCI_PORT_H
#define SORTIX_DISK_AHCI_PORT_H

#include <sys/types.h>

#include <endian.h>
#include <stdint.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/harddisk.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kthread.h>

namespace Sortix {
namespace AHCI {

struct command_header;
struct command_table;
struct fis;
struct port_regs;
struct prd;

class HBA;

class Port : public Harddisk
{
public:
	Port(HBA* hba, uint32_t port_index);
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
	void OnInterrupt();

private:
	__attribute__((format(printf, 2, 3)))
	void LogF(const char* format, ...);
	bool Reset();
	void Seek(blkcnt_t block_index, size_t count);
	void CommandDMA(uint8_t cmd, size_t size, bool write);
	bool FinishTransferDMA();
	void PrepareAwaitInterrupt();
	bool AwaitInterrupt(unsigned int msescs);

private:
	kthread_mutex_t port_lock;
	unsigned char identify_data[512];
	char serial[20 + 1];
	char revision[8 + 1];
	char model[40 + 1];
	addralloc_t control_alloc;
	addralloc_t dma_alloc;
	HBA* hba;
	volatile struct port_regs* regs;
	volatile struct command_header* clist;
	volatile struct fis* fis;
	volatile struct command_table* ctbl;
	volatile struct prd* prdt;
	addr_t control_physical_frame;
	addr_t dma_physical_frame;
	uint32_t port_index;
	bool is_control_page_mapped;
	bool is_dma_page_mapped;
	bool is_lba48;
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

} // namespace AHCI
} // namespace Sortix

#endif