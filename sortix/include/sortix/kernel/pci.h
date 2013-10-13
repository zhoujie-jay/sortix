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

	pci.h
	Functions for handling PCI devices.

*******************************************************************************/

#ifndef SORTIX_PCI_H
#define SORTIX_PCI_H

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
uint32_t SearchForDevice(pcifind_t pcifind);
addr_t ParseDevBar0(uint32_t devaddr);

} // namespace PCI
} // namespace Sortix

#endif
