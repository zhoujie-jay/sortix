/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * pci.cpp
 * Functions for handling PCI devices.
 */

#include <assert.h>
#include <endian.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/pci.h>

namespace Sortix {
namespace PCI {

static kthread_mutex_t pci_lock = KTHREAD_MUTEX_INITIALIZER;

const uint16_t CONFIG_ADDRESS = 0xCF8;
const uint16_t CONFIG_DATA = 0xCFC;

uint32_t MakeDevAddr(uint8_t bus, uint8_t slot, uint8_t func)
{
	//assert(bus < 1UL<<8UL); // bus is 8 bit anyways.
	assert(slot < 1UL<<5UL);
	assert(func < 1UL<<3UL);
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
	assert((off & 0x3) == 0);
	outport32(CONFIG_ADDRESS, devaddr + off);
	return inport32(CONFIG_DATA);
}

void WriteRaw32(uint32_t devaddr, uint8_t off, uint32_t val)
{
	assert((off & 0x3) == 0);
	outport32(CONFIG_ADDRESS, devaddr + off);
	outport32(CONFIG_DATA, val);
}

uint32_t Read32(uint32_t devaddr, uint8_t off)
{
	return le32toh(ReadRaw32(devaddr, off));
}

void Write32(uint32_t devaddr, uint8_t off, uint32_t val)
{
	WriteRaw32(devaddr, off, htole32(val));
}

void Write16(uint32_t devaddr, uint8_t off, uint16_t val)
{
	assert((off & 0x1) == 0);
	uint8_t alignedoff = off & ~0x3;
	union { uint8_t val8[4]; uint32_t val32; };
	val32 = ReadRaw32(devaddr, alignedoff);
	val8[(off & 0x3) + 0] = val >> 0 & 0xFF;
	val8[(off & 0x3) + 1] = val >> 8 & 0xFF;
	WriteRaw32(devaddr, alignedoff, val32);
}

uint16_t Read16(uint32_t devaddr, uint8_t off)
{
	assert((off & 0x1) == 0);
	uint8_t alignedoff = off & ~0x3;
	union { uint8_t val8[4]; uint32_t val32; };
	val32 = ReadRaw32(devaddr, alignedoff);
	return (uint16_t) val8[(off & 0x3) + 0] << 0 |
	       (uint16_t) val8[(off & 0x3) + 1] << 8;
}

void Write8(uint32_t devaddr, uint8_t off, uint8_t val)
{
	uint8_t alignedoff = off & ~0x3;
	union { uint8_t val8[4]; uint32_t val32; };
	val32 = ReadRaw32(devaddr, alignedoff);
	val8[(off & 0x3)] = val;
	WriteRaw32(devaddr, alignedoff, val32);
}

uint8_t Read8(uint32_t devaddr, uint8_t off)
{
	uint8_t alignedoff = off & ~0x3;
	union { uint8_t val8[4]; uint32_t val32; };
	val32 = ReadRaw32(devaddr, alignedoff);
	return val8[(off & 0x3)];
}

uint32_t CheckDevice(uint8_t bus, uint8_t slot, uint8_t func)
{
	return Read32(MakeDevAddr(bus, slot, func), 0x0);
}

pciid_t GetDeviceId(uint32_t devaddr)
{
	pciid_t ret;
	ret.deviceid = Read16(devaddr, PCIFIELD_DEVICE_ID);
	ret.vendorid = Read16(devaddr, PCIFIELD_VENDOR_ID);
	return ret;
}

pcitype_t GetDeviceType(uint32_t devaddr)
{
	pcitype_t ret;
	ret.classid = Read8(devaddr, PCIFIELD_CLASS);
	ret.subclassid = Read8(devaddr, PCIFIELD_SUBCLASS);
	ret.progif = Read8(devaddr, PCIFIELD_PROG_IF);
	ret.revid = Read8(devaddr, PCIFIELD_REVISION_ID);
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

// TODO: This iterates the whole PCI device tree on each call!
static uint32_t SearchForDevicesOnBus(uint8_t bus, pcifind_t pcifind, uint32_t last = 0)
{
	bool found_any_device = false;
	uint32_t next_device = 0;

	for ( unsigned int slot = 0; slot < 32; slot++ )
	{
		unsigned int num_functions = 1;
		for ( unsigned int function = 0; function < num_functions; function++ )
		{
			uint32_t devaddr = MakeDevAddr(bus, slot, function);
			if ( last < devaddr &&
			     (!found_any_device || devaddr < next_device) &&
			     MatchesSearchCriteria(devaddr, pcifind) )
				next_device = devaddr, found_any_device = true;
			uint8_t header = Read8(devaddr, PCIFIELD_HEADER_TYPE);
			if ( header & 0x80 ) // Multi function device.
				num_functions = 8;
			if ( (header & 0x7F) == 0x01 ) // PCI to PCI bus.
			{
				uint8_t subbusid = Read8(devaddr, PCIFIELD_SECONDARY_BUS_NUMBER);
				uint32_t recret = SearchForDevicesOnBus(subbusid, pcifind, last);
				if ( last < recret &&
				     (!found_any_device || recret < next_device) )
					next_device = recret, found_any_device = true;
			}
		}
	}

	if ( !found_any_device )
		return 0;

	return next_device;
}

uint32_t SearchForDevices(pcifind_t pcifind, uint32_t last)
{
	// Search on bus 0 and recurse on other detected busses.
	return SearchForDevicesOnBus(0, pcifind, last);
}

pcibar_t GetBAR(uint32_t devaddr, uint8_t bar)
{
	ScopedLock lock(&pci_lock);

	uint32_t low = PCI::Read32(devaddr, 0x10 + 4 * (bar+0));

	pcibar_t result;
	result.addr_raw = low;
	result.size_raw = 0;
	if ( result.is_64bit() )
	{
		uint32_t high = PCI::Read32(devaddr, 0x10 + 4 * (bar+1));
		result.addr_raw |= (uint64_t) high << 32;
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), 0xFFFFFFFF);
		PCI::Write32(devaddr, 0x10 + 4 * (bar+1), 0xFFFFFFFF);
		uint32_t size_low = PCI::Read32(devaddr, 0x10 + 4 * (bar+0));
		uint32_t size_high = PCI::Read32(devaddr, 0x10 + 4 * (bar+1));
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), low);
		PCI::Write32(devaddr, 0x10 + 4 * (bar+1), high);
		result.size_raw = (uint64_t) size_high << 32 | (uint64_t) size_low << 0;
		result.size_raw = ~(result.size_raw & 0xFFFFFFFFFFFFFFF0) + 1;
	}
	else if ( result.is_32bit() )
	{
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), 0xFFFFFFFF);
		uint32_t size_low = PCI::Read32(devaddr, 0x10 + 4 * (bar+0));
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), low);
		result.size_raw = (uint64_t) size_low << 0;
		result.size_raw = ~(result.size_raw & 0xFFFFFFF0) + 1;
		result.size_raw &= 0xFFFFFFFF;
	}
	else if ( result.is_iospace() )
	{
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), 0xFFFFFFFF);
		uint32_t size_low = PCI::Read32(devaddr, 0x10 + 4 * (bar+0));
		PCI::Write32(devaddr, 0x10 + 4 * (bar+0), low);
		result.size_raw = (uint64_t) size_low << 0;
		result.size_raw = ~(result.size_raw & 0xFFFFFFFC) + 1;
		result.size_raw &= 0xFFFFFFFF;
	}

	return result;
}

pcibar_t GetExpansionROM(uint32_t devaddr)
{
	const uint32_t ROM_ADDRESS_MASK = ~UINT32_C(0x7FF);

	ScopedLock lock(&pci_lock);

	uint32_t low = PCI::Read32(devaddr, 0x30);
	PCI::Write32(devaddr, 0x30, ROM_ADDRESS_MASK | low);
	uint32_t size_low = PCI::Read32(devaddr, 0x30);
	PCI::Write32(devaddr, 0x30, low);

	pcibar_t result;
	result.addr_raw = (low & ROM_ADDRESS_MASK) | PCIBAR_TYPE_32BIT;
	result.size_raw = ~(size_low & ROM_ADDRESS_MASK) + 1;
	return result;
}

void EnableExpansionROM(uint32_t devaddr)
{
	ScopedLock lock(&pci_lock);

	PCI::Write32(devaddr, 0x30, PCI::Read32(devaddr, 0x30) | 0x1);
}

void DisableExpansionROM(uint32_t devaddr)
{
	ScopedLock lock(&pci_lock);

	PCI::Write32(devaddr, 0x30, PCI::Read32(devaddr, 0x30) & ~UINT32_C(0x1));
}

bool IsExpansionROMEnabled(uint32_t devaddr)
{
	ScopedLock lock(&pci_lock);

	return PCI::Read32(devaddr, 0x30) & 0x1;
}

void Init()
{
}

} // namespace PCI
} // namespace Sortix
