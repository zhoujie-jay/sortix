/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    ata.cpp
    Allowes access to block devices over ATA PIO.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/cpu.h>
#include <sortix/stat.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "ata.h"

// TODO: Use the PCI to detect ATA devices instead of relying on them being on
// standard locations.

namespace Sortix {

const uint16_t PRIMARY_BUS_OFFSET = 0x1F0;
const uint16_t SECONDARY_BUS_OFFSET = 0x170;
const uint16_t DATA = 0x0;
const uint16_t FEATURE = 0x1;
const uint16_t ERROR = 0x1;
const uint16_t SECTOR_COUNT = 0x2;
const uint16_t LBA_LOW = 0x3;
const uint16_t LBA_MID = 0x4;
const uint16_t LBA_HIGH = 0x5;
const uint16_t DRIVE_SELECT = 0x6;
const uint16_t COMMAND = 0x7;
const uint16_t STATUS = 0x7;
const uint8_t CMD_READ = 0x20;
const uint8_t CMD_READ_EXT = 0x24;
const uint8_t CMD_WRITE = 0x30;
const uint8_t CMD_WRITE_EXT = 0x34;
const uint8_t CMD_FLUSH_CACHE = 0xE7;
const uint8_t CMD_IDENTIFY = 0xEC;
const uint8_t STATUS_ERROR = (1<<0);
const uint8_t STATUS_DATAREADY = (1<<3);
const uint8_t STATUS_DRIVEFAULT = (1<<5);
const uint8_t STATUS_BUSY = (1<<7);
const uint8_t CTL_NO_INTERRUPT = (1<<1);
const uint8_t CTL_RESET = (1<<2);

namespace ATA {

class ATANode : public AbstractInode
{
public:
	ATANode(ATADrive* drive, uid_t owner, gid_t group, mode_t mode, dev_t dev,
	        ino_t ino);
	virtual ~ATANode();
	virtual int sync(ioctx_t* ctx);
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);

private:
	kthread_mutex_t filelock;
	ATADrive* drive;

};

ATANode::ATANode(ATADrive* drive, uid_t owner, gid_t group, mode_t mode,
                 dev_t dev, ino_t /*ino*/)
{
	inode_type = INODE_TYPE_FILE;
	filelock = KTHREAD_MUTEX_INITIALIZER;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFBLK;
	this->stat_size = (off_t) drive->GetSize();
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_blksize = (blksize_t) drive->GetSectorSize();
	this->stat_blocks = (blkcnt_t) drive->GetNumSectors();
	this->drive = drive;
}

ATANode::~ATANode()
{
	delete drive;
}

int ATANode::sync(ioctx_t* /*ctx*/)
{
	// TODO: Actually sync the device here!
	return 0;
}

int ATANode::truncate(ioctx_t* /*ctx*/, off_t length)
{
	ScopedLock lock(&filelock);
	if ( length == drive->GetSize() ) { return 0; }
	errno = EPERM;
	return -1;
}

off_t ATANode::lseek(ioctx_t* /*ctx*/, off_t offset, int whence)
{
	ScopedLock lock(&filelock);
	if ( whence == SEEK_SET )
		return offset;
	if ( whence == SEEK_END )
		return (off_t) drive->GetSize() + offset;
	errno = EINVAL;
	return -1;
}

ssize_t ATANode::pread(ioctx_t* /*ctx*/, uint8_t* buf, size_t count, off_t off)
{
	// TODO: SECURITY: Use ioctx copy functions to copy or we have a serious
	// security hole if invoked from user-space!
	size_t numbytes = drive->Read(off, buf, count);
	if ( numbytes < count )
		return -1;
	return (ssize_t) numbytes;
}

ssize_t ATANode::pwrite(ioctx_t* /*ctx*/, const uint8_t* buf, size_t count,
                        off_t off)
{
	// TODO: SECURITY: Use ioctx copy functions to copy or we have a serious
	// security hole if invoked from user-space!
	size_t numbytes = drive->Write(off, buf, count);
	if ( numbytes < count )
		return -1;
	return (ssize_t) numbytes;
}

void DetectDrive(const char* devpath, Ref<Descriptor> slashdev, unsigned busid,
                 ATABus* bus, unsigned driveid)
{
	unsigned ataid = busid*2 + driveid;
	ATADrive* drive = bus->Instatiate(driveid);
	if ( !drive )
		return;
	Ref<ATANode> node(new ATANode(drive, 0, 0, 0660, slashdev->dev, 0));
	if ( !node )
		Panic("Unable to allocate memory for ATA drive inode.");
	const size_t NAMELEN = 64;
	char name[NAMELEN];
	snprintf(name, NAMELEN, "ata%u", ataid);
	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	if ( LinkInodeInDir(&ctx, slashdev, name, node) != 0 )
		PanicF("Unable to link %s/%s to ATA driver inode.", devpath, name);
}

void DetectBus(const char* devpath, Ref<Descriptor> slashdev, unsigned busid,
               uint16_t ioport, uint16_t altio)
{
	ATABus* bus = ATA::CreateBus(ioport, altio);
	if ( !bus )
		return;
	DetectDrive(devpath, slashdev, busid, bus, 0);
	DetectDrive(devpath, slashdev, busid, bus, 1);
}

void Init(const char* devpath, Ref<Descriptor> slashdev)
{
	DetectBus(devpath, slashdev, 0, 0x1F0, 0x3F6);
	DetectBus(devpath, slashdev, 1, 0x170, 0x366);
}

ATABus* CreateBus(uint16_t portoffset, uint16_t altport)
{
	unsigned status = CPU::InPortB(portoffset + STATUS);
	// Detect if there is no such bus.
	if ( status == 0xFF )
	{
		errno = ENODEV;
		return NULL;
	}
	return new ATABus(portoffset, altport);
}

} // namespace ATA

void Wait400NSecs(uint16_t iobase)
{
	// Now wait 400 ns for the drive to be ready.
	for ( unsigned i = 0; i < 4; i++ ) { CPU::InPortB(iobase + STATUS); }
}

ATABus::ATABus(uint16_t portoffset, uint16_t altport)
{
	this->iobase = portoffset;
	this->altport = altport;
	this->curdriveid = 0;
}

ATABus::~ATABus()
{
}

ATADrive* ATABus::Instatiate(unsigned driveid)
{
	if ( 1 < driveid )
		return errno = EINVAL, (ATADrive*) NULL;
	curdriveid = 0;

	uint8_t drivemagic = 0xA0 | (driveid << 4);
	CPU::OutPortB(iobase + DRIVE_SELECT, drivemagic);
	CPU::OutPortB(iobase + SECTOR_COUNT, 0);
	CPU::OutPortB(iobase + LBA_LOW, 0);
	CPU::OutPortB(iobase + LBA_MID, 0);
	CPU::OutPortB(iobase + LBA_HIGH, 0);
	CPU::OutPortB(iobase + COMMAND, CMD_IDENTIFY);
	uint8_t status;
	while ( true )
	{
		status = CPU::InPortB(iobase + STATUS);
		if ( !status || status == 0xFF )
			return errno = ENODEV, (ATADrive*) NULL;
		if ( !(status & STATUS_BUSY) )
			break;
	}
	// Check for ATAPI device not following spec.
	if ( CPU::InPortB(iobase + LBA_MID) || CPU::InPortB(iobase + LBA_MID) )
		return errno = ENODEV, (ATADrive*) NULL;
	while ( !(status & STATUS_DATAREADY) && !(status & STATUS_ERROR) )
		status = CPU::InPortB(iobase + STATUS);
	if ( status & STATUS_ERROR )
	{
		unsigned mid = CPU::InPortB(iobase + LBA_MID);
		unsigned high = CPU::InPortB(iobase + LBA_HIGH);
		if ( mid == 0x14 && high == 0xEB )
		{
			//Log::PrintF("Found ATAPI device instead of ATA\n");
		}
		else if ( mid == 0x3C && high == 0xC3 )
		{
			//Log::PrintF("Found SATA device instead of ATA\n");
		}
		else if ( mid || high )
		{
			//Log::PrintF("Found unknown device instead of ATA\n");
		}
		else
		{
			//Log::PrintF("Error status during identify\n");
		}
		return errno = EIO, (ATADrive*) NULL;
	}
	ATADrive* drive = new ATADrive(this, driveid, iobase, altport);
	return drive;
}

bool ATABus::SelectDrive(unsigned driveid)
{
	if ( driveid == curdriveid ) { return true; }
	if ( 1 < driveid ) { errno = EINVAL; return false; }

	uint8_t drivemagic = 0xA0 | (driveid << 4);
	CPU::OutPortB(iobase + DRIVE_SELECT, drivemagic);
	Wait400NSecs(iobase);
	return true;
}

const size_t META_LBA28 = 60;
const size_t META_FLAGS = 83;
const size_t META_LBA48 = 100;
const uint16_t FLAG_LBA48 = (1<<10);

ATADrive::ATADrive(ATABus* bus, unsigned driveid, uint16_t portoffset, uint16_t altport)
{
	this->atalock = KTHREAD_MUTEX_INITIALIZER;
	this->bus = bus;
	this->driveid = driveid;
	this->iobase = portoffset;
	this->altport = altport;
	for ( size_t i = 0; i < 256; i++ )
	{
		meta[i] = CPU::InPortW(iobase + DATA);
	}
	lba48 = meta[META_FLAGS] & FLAG_LBA48;
	numsectors = 0;
	if ( lba48 )
	{
		numsectors = (uint64_t) meta[META_LBA48 + 0] <<  0
		           | (uint64_t) meta[META_LBA48 + 1] <<  8
		           | (uint64_t) meta[META_LBA48 + 2] << 16
		           | (uint64_t) meta[META_LBA48 + 3] << 24
		           | (uint64_t) meta[META_LBA48 + 4] << 32
		           | (uint64_t) meta[META_LBA48 + 5] << 40
		           | (uint64_t) meta[META_LBA48 + 6] << 48
		           | (uint64_t) meta[META_LBA48 + 7] << 56;
	}
	else
	{
		numsectors = meta[META_LBA28 + 0] <<  0
		           | meta[META_LBA28 + 1] <<  8
		           | meta[META_LBA28 + 2] << 16
		           | meta[META_LBA28 + 3] << 24;
	}
	sectorsize = 512; // TODO: Detect this!
	Initialize();
}

ATADrive::~ATADrive()
{
}

off_t ATADrive::GetSectorSize()
{
	return sectorsize;
}

off_t ATADrive::GetNumSectors()
{
	return numsectors;
}

bool ATADrive::PrepareIO(bool write, off_t sector)
{
	if ( numsectors <= sector ) { errno = EINVAL; return false; }
	if ( write && !ENABLE_DISKWRITE )
	{
		errno = EPERM;
		return false;
	}
	bus->SelectDrive(driveid);
	uint8_t mode = (lba48) ? 0x40 : 0xE0;
	mode |= driveid << 4;
	mode |= (lba48) ? 0 : (sector >> 24) & 0x0F;
	CPU::OutPortB(iobase + DRIVE_SELECT, mode);
	uint16_t sectorcount = 1;
	uint8_t sectorcountlow = sectorcount & 0xFF;
	uint8_t sectorcounthigh = (sectorcount >> 8) & 0xFF;
	if ( lba48 )
	{
		CPU::OutPortB(iobase + SECTOR_COUNT, sectorcounthigh);
		CPU::OutPortB(iobase + LBA_LOW, (sector >> 24) & 0xFF);
		CPU::OutPortB(iobase + LBA_MID, (sector >> 32) & 0xFF);
		CPU::OutPortB(iobase + LBA_HIGH, (sector >> 40) & 0xFF);
	}
	CPU::OutPortB(iobase + SECTOR_COUNT, sectorcountlow);
	CPU::OutPortB(iobase + LBA_LOW, sector & 0xFF);
	CPU::OutPortB(iobase + LBA_MID, (sector >> 8) & 0xFF);
	CPU::OutPortB(iobase + LBA_HIGH, (sector >> 16) & 0xFF);
	uint8_t command = (write) ? CMD_WRITE : CMD_READ;
	if ( lba48 ) { command = (write) ? CMD_WRITE_EXT : CMD_READ_EXT; }
	CPU::OutPortB(iobase + COMMAND, command);
	while ( true )
	{
		uint8_t status = CPU::InPortB(iobase + STATUS);
		if ( status & STATUS_BUSY ) { continue; }
		if ( status & STATUS_DATAREADY ) { break; }
		if ( status & STATUS_ERROR ) { errno = EIO; return false; }
		if ( status & STATUS_DRIVEFAULT ) { errno = EIO; return false; }
	}
	return true;
}

bool ATADrive::ReadSector(off_t sector, uint8_t* dest)
{
	ScopedLock lock(&atalock);
	if ( !PrepareIO(false, sector) ) { return false; }
	uint16_t* destword = (uint16_t*) dest;
	for ( size_t i = 0; i < sectorsize/2; i++ )
	{
		destword[i] = CPU::InPortW(iobase + DATA);
	}
	Wait400NSecs(iobase);
	CPU::InPortB(iobase + STATUS);
	return true;
}

bool ATADrive::WriteSector(off_t sector, const uint8_t* src)
{
	ScopedLock lock(&atalock);
	if ( !PrepareIO(true, sector) ) { return false; }
	const uint16_t* srcword = (const uint16_t*) src;
	for ( size_t i = 0; i < sectorsize/2; i++ )
	{
		CPU::OutPortW(iobase + DATA, srcword[i]);
	}
	Wait400NSecs(iobase);
	CPU::OutPortB(iobase + COMMAND, CMD_FLUSH_CACHE);
	while ( true )
	{
		uint8_t status = CPU::InPortB(iobase + STATUS);
		if ( status & STATUS_ERROR ) { errno = EIO; return false; }
		if ( status & STATUS_DRIVEFAULT ) { errno = EIO; return false; }
		if ( !(status & STATUS_BUSY) ) { break; }
	}
	return true;
}

size_t ATADrive::Read(off_t byteoffset, uint8_t* dest, size_t numbytes)
{
	size_t sofar = 0;
	size_t leadingbytes = byteoffset % sectorsize;
	if ( leadingbytes || numbytes < sectorsize )
	{
		size_t wanted = sectorsize - leadingbytes;
		if ( numbytes < wanted ) { wanted = numbytes; }
		uint8_t temp[512 /*sectorsize*/];
		if ( !ReadSector(byteoffset/sectorsize, temp) ) { return sofar; }
		memcpy(dest + sofar, temp + leadingbytes, wanted);
		sofar += wanted;
		numbytes -= wanted;
		byteoffset += wanted;
	}

	while ( sectorsize <= numbytes )
	{
		if ( !ReadSector(byteoffset/sectorsize, dest + sofar) ) { return sofar; }
		sofar += sectorsize;
		numbytes -= sectorsize;
		byteoffset += sectorsize;
	}

	if ( numbytes ) { return sofar + Read(byteoffset, dest + sofar, numbytes); }
	return sofar;
}

size_t ATADrive::Write(off_t byteoffset, const uint8_t* src, size_t numbytes)
{
	size_t sofar = 0;
	size_t leadingbytes = byteoffset % sectorsize;
	if ( leadingbytes || numbytes < sectorsize )
	{
		size_t wanted = sectorsize - leadingbytes;
		if ( numbytes < wanted ) { wanted = numbytes; }
		uint8_t temp[512 /*sectorsize*/];
		if ( !ReadSector(byteoffset/sectorsize, temp) ) { return sofar; }
		memcpy(temp + leadingbytes, src + sofar, wanted);
		if ( !WriteSector(byteoffset/sectorsize, temp) ) { return sofar; }
		sofar += wanted;
		numbytes -= wanted;
		byteoffset += wanted;
	}

	while ( sectorsize <= numbytes )
	{
		if ( !WriteSector(byteoffset/sectorsize, src + sofar) ) { return sofar; }
		sofar += sectorsize;
		numbytes -= sectorsize;
		byteoffset += sectorsize;
	}

	if ( numbytes ) { return sofar + Write(byteoffset, src + sofar, numbytes); }
	return sofar;
}

void ATADrive::Initialize()
{
	bus->SelectDrive(driveid);
	CPU::OutPortB(iobase + COMMAND, CTL_NO_INTERRUPT);
}

} // namespace Sortix
