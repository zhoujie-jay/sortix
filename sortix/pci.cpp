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

	pci.cpp
	Functions for handling PCI devices.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/endian.h>
#include <sortix/kernel/pci.h>
#include "cpu.h" // TODO: Put this in some <sortix/kernel/cpu.h>

// TODO: Verify that the endian conversions in this file actually works. I have
// a sneaking suspicion that they won't work on non-little endian platforms.

namespace Sortix {
namespace PCI {

const uint16_t CONFIG_ADDRESS = 0xCF8;
const uint16_t CONFIG_DATA = 0xCFC;

uint32_t MakeDevAddr(uint8_t bus, uint8_t slot, uint8_t func)
{
	//ASSERT(bus < 1UL<<8UL); // bus is 8 bit anyways.
	ASSERT(slot < 1UL<<5UL);
	ASSERT(func < 1UL<<3UL);
	return func << 8U | slot << 11U | bus << 16U | 1 << 31U;
}

void SplitDevAddr(uint32_t devaddr, uint8_t* vals /* bus, slot, func */)
{
	vals[0] = devaddr >> 16U & ((1UL<<8UL)-1);
	vals[1] = devaddr >> 11U & ((1UL<<3UL)-1);
	vals[2] = devaddr >>  8U & ((1UL<<5UL)-1);
}

uint32_t ReadRaw32(uint32_t devaddr, uint8_t off)
{
	CPU::OutPortL(CONFIG_ADDRESS, devaddr + off);
	return CPU::InPortL(CONFIG_DATA);
}

void WriteRaw32(uint32_t devaddr, uint8_t off, uint32_t val)
{
	CPU::OutPortL(CONFIG_ADDRESS, devaddr + off);
	CPU::OutPortL(CONFIG_DATA, val);
}

uint32_t Read32(uint32_t devaddr, uint8_t off)
{
	return LittleToHost(ReadRaw32(devaddr, off));
}

void Write32(uint32_t devaddr, uint8_t off, uint32_t val)
{
	WriteRaw32(devaddr, off, HostToLittle(val));
}

uint16_t Read16(uint32_t devaddr, uint8_t off)
{
	ASSERT((off & 0x1) == 0);
	uint8_t alignedoff = off & ~0x3;
	union { uint16_t val16[2]; uint32_t val32; };
	val32 = ReadRaw32(devaddr, alignedoff);
	uint16_t ret = off & 0x2 ? val16[0] : val16[1];
	return LittleToHost(ret);
}

uint8_t Read8(uint32_t devaddr, uint8_t off)
{
	uint8_t alignedoff = off & ~0x1;
	union { uint8_t val8[2]; uint32_t val16; };
	val16 = HostToLittle(Read16(devaddr, alignedoff));
	uint8_t ret = off & 0x1 ? val8[0] : val8[1];
	return ret;
}

uint32_t CheckDevice(uint8_t bus, uint8_t slot, uint8_t func)
{
	return Read32(MakeDevAddr(bus, slot, func), 0x0);
}

pciid_t GetDeviceId(uint32_t devaddr)
{
	pciid_t ret;
	ret.deviceid = Read16(devaddr, 0x00);
	ret.vendorid = Read16(devaddr, 0x02);
	return ret;
}

pcitype_t GetDeviceType(uint32_t devaddr)
{
	pcitype_t ret;
	ret.classid = Read8(devaddr, 0x08);
	ret.subclassid = Read8(devaddr, 0x09);
	ret.progif = Read8(devaddr, 0x0A);
	ret.revid = Read8(devaddr, 0x0B);
	return ret;
}

static bool MatchesSearchCriteria(uint32_t devaddr, pcifind_t pcifind)
{
	pciid_t id = GetDeviceId(devaddr);
	if ( id.vendorid == 0xFFFF && id.deviceid == 0xFFFF )
		return false;
	pcitype_t type = GetDeviceType(devaddr);
	if ( pcifind.vendorid != 0xFFFF && id.vendorid != pcifind.vendorid )
		return false;
	if ( pcifind.deviceid != 0xFFFF && id.deviceid != pcifind.deviceid )
		return false;
	if ( pcifind.classid != 0xFF && type.classid != pcifind.classid )
		return false;
	if ( pcifind.subclassid != 0xFF && type.subclassid != pcifind.subclassid )
		return false;
	if ( pcifind.progif != 0xFF && type.progif != pcifind.progif )
		return false;
	if ( pcifind.revid != 0xFF && type.revid != pcifind.revid )
		return false;
	return true;
}

static uint32_t SearchForDeviceOnBus(uint8_t bus, pcifind_t pcifind)
{
	for ( unsigned slot = 0; slot < 32; slot++ )
	{
		unsigned numfuncs = 1;
		for ( unsigned func = 0; func < numfuncs; func++ )
		{
			uint32_t devaddr = MakeDevAddr(bus, slot, func);
			if ( MatchesSearchCriteria(devaddr, pcifind) )
				return devaddr;
			uint8_t header = Read8(devaddr, 0x0D); // Secondary Bus Number.
			if ( header & 0x80 ) // Multi function device.
				numfuncs = 8;
			if ( (header & 0x7F) == 0x01 ) // PCI to PCI bus.
			{
				uint8_t subbusid = Read8(devaddr, 0x1A);
				uint32_t recret = SearchForDeviceOnBus(subbusid, pcifind);
				if ( recret )
					return recret;
			}
		}
	}
	return 0;
}

uint32_t SearchForDevice(pcifind_t pcifind)
{
	// Search on bus 0 and recurse on other detected busses.
	return SearchForDeviceOnBus(0, pcifind);
}

// TODO: This is just a hack but will do for now.
addr_t ParseDevBar0(uint32_t devaddr)
{
	uint32_t bar0 = Read32(devaddr, 0x10);
	if ( bar0 & 0x1 ) // IO Space
		return bar0 & ~0x7UL;
	else // Memory Space
	{
		//uint32_t type = bar0 >> 1 & 0x3;
		//uint32_t prefetchable = bar0 >> 3 & 0x1;
		//if ( type == 0x01 )
		//	// TODO: Support 16-bit addresses here.
		//if ( type == 0x02 )
		//	// TODO: Support 64-bit addresses here.
		return bar0 & ~0xFUL;
	}
}

void Init()
{
}

} // namespace PCI
} // namespace Sortix
