/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    ata.h
    Allowes access to block devices over ATA PIO.

*******************************************************************************/

#ifndef SORTIX_ATA_H
#define SORTIX_ATA_H

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

class Descriptor;
class ATABus;
class ATADrive;

class ATABus
{
public:
	ATABus(uint16_t portoffset, uint16_t altport);
	~ATABus();

public:
	ATADrive* Instatiate(unsigned driveid);
	bool SelectDrive(unsigned driveid);

private:
	unsigned curdriveid;
	uint16_t iobase;
	uint16_t altport;

};

class ATADrive
{
public:
	off_t GetSectorSize();
	off_t GetNumSectors();
	off_t GetSize() { return GetSectorSize() * GetNumSectors(); }
	bool ReadSector(off_t sector, uint8_t* dest);
	bool WriteSector(off_t sector, const uint8_t* src);
	size_t Read(off_t byteoffset, uint8_t* dest, size_t numbytes);
	size_t Write(off_t byteoffset, const uint8_t* src, size_t numbytes);

public:
	ATADrive(ATABus* bus, unsigned driveid, uint16_t portoffset, uint16_t altport);
	~ATADrive();

private:
	void Initialize();
	bool PrepareIO(bool write, off_t sector);

private:
	kthread_mutex_t atalock;
	unsigned driveid;
	uint16_t meta[256];
	uint16_t iobase;
	uint16_t altport;
	ATABus* bus;
	bool lba48;
	size_t sectorsize;
	off_t numsectors;

};

namespace ATA {

void Init(const char* devpath, Ref<Descriptor> slashdev);
ATABus* CreateBus(uint16_t portoffset, uint16_t altport);

} // namespace ATA

} // namespace Sortix

#endif
