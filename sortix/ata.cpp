/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	ata.cpp
	Allowes access to block devices over ATA PIO.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "ata.h"
#include "fs/devfs.h"

// TODO: Use the PCI to detect ATA devices instead of relying on them being on
// standard locations.

using namespace Maxsi;

namespace Sortix
{
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

	namespace ATA
	{
		void DetectDrive(unsigned busid, ATABus* bus, unsigned driveid)
		{
			unsigned ataid = busid*2 + driveid;
			ATADrive* drive = bus->Instatiate(driveid);
			if ( !drive ) { return; }
			DeviceFS::RegisterATADrive(ataid, drive);
		}

		void DetectBus(unsigned busid, uint16_t ioport, uint16_t altio)
		{
			ATABus* bus = ATA::CreateBus(ioport, altio);
			DetectDrive(busid, bus, 0);
			DetectDrive(busid, bus, 1);
		}

		void Init()
		{
			DetectBus(0, 0x1F0, 0x3F6);
			DetectBus(1, 0x170, 0x366);
		}

		ATABus* CreateBus(uint16_t portoffset, uint16_t altport)
		{
			unsigned status = CPU::InPortB(portoffset + STATUS);
			// Detect if there is no such bus.
			if ( status == 0xFF )
			{
				Error::Set(ENODEV);
				return NULL;
			}
			return new ATABus(portoffset, altport);
		}
	}

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
		if ( 1 < driveid ) { Error::Set(EINVAL); return false; }
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
			if ( !status || status == 0xFF ) { Error::Set(ENODEV); return false; }
			if ( !(status & STATUS_BUSY) ) { break; }
		}
		if ( CPU::InPortB(iobase + LBA_MID) || CPU::InPortB(iobase + LBA_MID) )
		{
			Error::Set(ENODEV); return false; // ATAPI device not following spec.
		}
		while ( !(status & STATUS_DATAREADY) && !(status & STATUS_ERROR) )
		{
			status = CPU::InPortB(iobase + STATUS);
		}
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
			Error::Set(EIO);
			return false;
		}
		ATADrive* drive = new ATADrive(this, driveid, iobase, altport);
		return drive;
	}

	bool ATABus::SelectDrive(unsigned driveid)
	{
		if ( driveid == curdriveid ) { return true; }
		if ( 1 < driveid ) { Error::Set(EINVAL); return false; }

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
		this->bus = bus;
		this->driveid = driveid;
		this->iobase = portoffset;
		this->altport = altport;
		for ( size_t i = 0; i < 256; i++ )
		{
			meta[i] = CPU::InPortW(iobase + DATA);
		}
		lba48 = meta[META_FLAGS] & FLAG_LBA48;
		if ( lba48 )
		{
			numsectors = *((uint64_t*) (meta + META_LBA48));
		}
		else
		{
			numsectors = *((uint32_t*) (meta + META_LBA28));
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
		if ( numsectors <= sector ) { Error::Set(EINVAL); return false; }
		if ( write && !ENABLE_DISKWRITE )
		{
			Error::Set(EPERM);
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
			if ( status & STATUS_ERROR ) { Error::Set(EIO); return false; }
			if ( status & STATUS_DRIVEFAULT ) { Error::Set(EIO); return false; }
		}
		return true;
	}

	bool ATADrive::ReadSector(off_t sector, uint8_t* dest)
	{
		if ( !PrepareIO(false, sector) ) { return false; }
		uint16_t* destword = (uint16_t*) dest;
		for ( size_t i = 0; i < sectorsize/2; i++ )
		{
			destword[i] = CPU::InPortW(iobase + DATA);
		}
		Wait400NSecs(iobase);
		uint8_t status = CPU::InPortB(iobase + STATUS);
		return true;
	}

	bool ATADrive::WriteSector(off_t sector, const uint8_t* src)
	{
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
			if ( status & STATUS_ERROR ) { Error::Set(EIO); return false; }
			if ( status & STATUS_DRIVEFAULT ) { Error::Set(EIO); return false; }
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
			Memory::Copy(dest + sofar, temp + leadingbytes, wanted);
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
			Memory::Copy(temp + leadingbytes, src + sofar, wanted);
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
}

