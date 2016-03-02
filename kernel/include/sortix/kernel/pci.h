/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/pci.h
 * Functions for handling PCI devices.
 */

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

static const uint8_t PCIBAR_TYPE_IOSPACE = 0x0 << 1 | 0x1 << 0;
static const uint8_t PCIBAR_TYPE_16BIT = 0x1 << 1 | 0x0 << 0;
static const uint8_t PCIBAR_TYPE_32BIT = 0x0 << 1 | 0x0 << 0;
static const uint8_t PCIBAR_TYPE_64BIT = 0x2 << 1 | 0x0 << 0;

typedef struct
{
public:
	uint64_t addr_raw;
	uint64_t size_raw;

public:
	uint64_t addr() const { return is_iospace() ?
	                        addr_raw & 0xFFFFFFFFFFFFFFFC :
	                        addr_raw & 0xFFFFFFFFFFFFFFF0; }
	uint64_t size() const { return size_raw & 0xFFFFFFFFFFFFFFFF; }
	uint8_t type() const { return (addr_raw & 3) == PCIBAR_TYPE_IOSPACE ?
	                              (addr_raw & 3) : (addr_raw & 0x7);  }
	uint32_t ioaddr() const { return addr_raw & 0xFFFFFFFC; };
	bool is_prefetchable() const { return addr_raw & 0x8; }
	bool is_iospace() const { return type() == PCIBAR_TYPE_IOSPACE; }
	bool is_16bit() const { return type() == PCIBAR_TYPE_16BIT; }
	bool is_32bit() const { return type() == PCIBAR_TYPE_32BIT; }
	bool is_64bit() const { return type() == PCIBAR_TYPE_64BIT; }
	bool is_mmio() const { return is_16bit() || is_32bit() || is_64bit(); }
} pcibar_t;

static const uint8_t PCIFIELD_VENDOR_ID = 0x00;
static const uint8_t PCIFIELD_DEVICE_ID = 0x02;
static const uint8_t PCIFIELD_COMMAND = 0x04;
static const uint8_t PCIFIELD_STATUS = 0x06;
static const uint8_t PCIFIELD_REVISION_ID = 0x08;
static const uint8_t PCIFIELD_PROG_IF = 0x09;
static const uint8_t PCIFIELD_SUBCLASS = 0x0A;
static const uint8_t PCIFIELD_CLASS = 0x0B;
static const uint8_t PCIFIELD_CACHE_LINE_SIZE = 0x0C;
static const uint8_t PCIFIELD_LATENCY_TIMER = 0x0D;
static const uint8_t PCIFIELD_HEADER_TYPE = 0x0E;
static const uint8_t PCIFIELD_BIST = 0x0F;
static const uint8_t PCIFIELD_RAW_BAR0 = 0x10;
static const uint8_t PCIFIELD_RAW_BAR1 = 0x14;
static const uint8_t PCIFIELD_RAW_BAR2 = 0x18;
static const uint8_t PCIFIELD_PRIMARY_BUS_NUMBER = 0x18;
static const uint8_t PCIFIELD_SECONDARY_BUS_NUMBER = 0x19;
static const uint8_t PCIFIELD_SUBORDINATE_BUS_NUMBER = 0x1A;
static const uint8_t PCIFIELD_SECONDARY_LATENCY_TIMER = 0x1B;
static const uint8_t PCIFIELD_RAW_BAR3 = 0x1C;
static const uint8_t PCIFIELD_IO_BASE = 0x1C;
static const uint8_t PCIFIELD_IO_LIMIT = 0x1D;
static const uint8_t PCIFIELD_SECONDARY_STATUS = 0x1E;
static const uint8_t PCIFIELD_RAW_BAR4 = 0x20;
static const uint8_t PCIFIELD_MEMORY_BASE = 0x20;
static const uint8_t PCIFIELD_MEMORY_LIMIT = 0x22;
static const uint8_t PCIFIELD_RAW_BAR5 = 0x24;
static const uint8_t PCIFIELD_PREFETCHABLE_MEMORY_BASE = 0x24;
static const uint8_t PCIFIELD_PREFETCHABLE_MEMORY_LIMIT = 0x26;
static const uint8_t PCIFIELD_CARDBUS_CIS_POINTER = 0x28;
static const uint8_t PCIFIELD_PREFETCHABLE_BASE_UPPER_BITS = 0x28;
static const uint8_t PCIFIELD_SUBSYSTEM_VENDOR_ID = 0x2C;
static const uint8_t PCIFIELD_PREFETCHABLE_LIMIT_UPPER_BITS = 0x2C;
static const uint8_t PCIFIELD_SUBSYSTEM_ID = 0x2E;
static const uint8_t PCIFIELD_EXPANSION_ROM_BASE_ADDRESS = 0x30;
static const uint8_t PCIFIELD_CAPABILITIES = 0x34;
static const uint8_t PCIFIELD_EXPANSION_ROM_BASE_ADDRESS_PCI_BRIDGE = 0x38;
static const uint8_t PCIFIELD_INTERRUPT_LINE = 0x3C;
static const uint8_t PCIFIELD_INTERRUPT_PIN = 0x3D;
static const uint8_t PCIFIELD_MIN_GRANT = 0x3E;
static const uint8_t PCIFIELD_MAX_LATENCY = 0x3F;
static const uint8_t PCIFIELD_BRIDGE_CONTROL = 0x3E;

static const uint16_t PCIFIELD_COMMAND_IO_SPACE = 1 << 0;
static const uint16_t PCIFIELD_COMMAND_MEMORY_SPACE = 1 << 1;
static const uint16_t PCIFIELD_COMMAND_BUS_MASTER = 1 << 2;
static const uint16_t PCIFIELD_COMMAND_SPECIAL_CYCLES = 1 << 3;
static const uint16_t PCIFIELD_COMMAND_MEMORY_WRITE_AND_INVALIDATE = 1 << 4;
static const uint16_t PCIFIELD_COMMAND_VGA_PALETTE_SNOOP = 1 << 5;
static const uint16_t PCIFIELD_COMMAND_PARITY_ERROR_RESPONSE = 1 << 6;
static const uint16_t PCIFIELD_COMMAND_SERR = 1 << 8;
static const uint16_t PCIFIELD_COMMAND_FAST_BACK_TO_BACK = 1 << 9;
static const uint16_t PCIFIELD_COMMAND_INTERRUPT_DISABLE = 1 << 10;

namespace PCI {

void Init();
uint32_t MakeDevAddr(uint8_t bus, uint8_t slot, uint8_t func);
void SplitDevAddr(uint32_t devaddr, uint8_t* vals /* bus, slot, func */);
uint8_t Read8(uint32_t devaddr, uint8_t off); // Host endian
uint16_t Read16(uint32_t devaddr, uint8_t off); // Host endian
uint32_t Read32(uint32_t devaddr, uint8_t off); // Host endian
uint32_t ReadRaw32(uint32_t devaddr, uint8_t off); // PCI endian
void Write8(uint32_t devaddr, uint8_t off, uint8_t val); // Host endian
void Write16(uint32_t devaddr, uint8_t off, uint16_t val); // Host endian
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
