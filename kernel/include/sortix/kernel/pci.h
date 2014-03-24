/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    sortix/kernel/pci.h
    Functions for handling PCI devices.

*******************************************************************************/

#ifndef SORTIX_PCI_H
#define SORTIX_PCI_H

#include <endian.h>
#include <stdint.h>

namespace Sortix {

typedef struct
{
	uint16_t deviceid;
	uint16_t vendorid;
} pciid_t;

typedef struct
{
	uint8_t classid;
	uint8_t subclassid;
	uint8_t progif;
	uint8_t revid;
} pcitype_t;

// memset(&pcifind, 255, sizeof(pcifind)) and fill out rest.
typedef struct
{
	uint16_t deviceid;
	uint16_t vendorid;
	uint8_t classid;
	uint8_t subclassid;
	uint8_t progif;
	uint8_t revid;
} pcifind_t;

const uint8_t PCIBAR_TYPE_IOSPACE = 0x0 << 1 | 0x1 << 0;
const uint8_t PCIBAR_TYPE_16BIT = 0x1 << 1 | 0x0 << 0;
const uint8_t PCIBAR_TYPE_32BIT = 0x0 << 1 | 0x0 << 0;
const uint8_t PCIBAR_TYPE_64BIT = 0x2 << 1 | 0x0 << 0;

typedef struct
{
public:
	uint64_t addr_raw;
	uint64_t size_raw;

public:
	uint64_t addr() const { return addr_raw & 0xFFFFFFFFFFFFFFF0; }
	uint64_t size() const { return size_raw & 0xFFFFFFFFFFFFFFFF; }
	uint8_t type() const { return addr_raw & 0x7;  }
	uint32_t ioaddr() const { return addr_raw & 0xFFFFFFFC; };
	bool is_prefetchable() const { return addr_raw & 0x8; }
	bool is_iospace() const { return type() == PCIBAR_TYPE_IOSPACE; }
	bool is_16bit() const { return type() == PCIBAR_TYPE_16BIT; }
	bool is_32bit() const { return type() == PCIBAR_TYPE_32BIT; }
	bool is_64bit() const { return type() == PCIBAR_TYPE_64BIT; }
	bool is_mmio() const { return is_16bit() || is_32bit() || is_64bit(); }
} pcibar_t;

namespace PCI {

void Init();
uint32_t MakeDevAddr(uint8_t bus, uint8_t slot, uint8_t func);
void SplitDevAddr(uint32_t devaddr, uint8_t* vals /* bus, slot, func */);
uint8_t Read8(uint32_t devaddr, uint8_t off); // Host endian
uint16_t Read16(uint32_t devaddr, uint8_t off); // Host endian
uint32_t Read32(uint32_t devaddr, uint8_t off); // Host endian
uint32_t ReadRaw32(uint32_t devaddr, uint8_t off); // PCI endian
void Write32(uint32_t devaddr, uint8_t off, uint32_t val); // Host endian
void WriteRaw32(uint32_t devaddr, uint8_t off, uint32_t val); // PCI endian
pciid_t GetDeviceId(uint32_t devaddr);
pcitype_t GetDeviceType(uint32_t devaddr);
uint32_t SearchForDevices(pcifind_t pcifind, uint32_t last = 0);
pcibar_t GetBAR(uint32_t devaddr, uint8_t bar);
pcibar_t GetExpansionROM(uint32_t devaddr);
void EnableExpansionROM(uint32_t devaddr);
void DisableExpansionROM(uint32_t devaddr);
bool IsExpansionROMEnabled(uint32_t devaddr);

} // namespace PCI
} // namespace Sortix

#endif
