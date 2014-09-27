/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015, 2016.

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

    disk/ata/registers.h
    Driver for ATA.

*******************************************************************************/

#ifndef SORTIX_DISK_ATA_REGISTERS_H
#define SORTIX_DISK_ATA_REGISTERS_H

#include <endian.h>
#include <stdint.h>

namespace Sortix {
namespace ATA {

struct prd
{
	little_uint32_t physical;
	little_uint16_t count;
	little_uint16_t flags;
};

static const uint16_t PRD_FLAG_EOT = 1 << 15;

static const uint16_t REG_DATA = 0x0;
static const uint16_t REG_FEATURE = 0x1;
static const uint16_t REG_ERROR = 0x1;
static const uint16_t REG_SECTOR_COUNT = 0x2;
static const uint16_t REG_LBA_LOW = 0x3;
static const uint16_t REG_LBA_MID = 0x4;
static const uint16_t REG_LBA_HIGH = 0x5;
static const uint16_t REG_DRIVE_SELECT = 0x6;
static const uint16_t REG_COMMAND = 0x7;
static const uint16_t REG_STATUS = 0x7;

static const uint8_t CMD_READ = 0x20;
static const uint8_t CMD_READ_EXT = 0x24;
static const uint8_t CMD_READ_DMA = 0xC8;
static const uint8_t CMD_READ_DMA_EXT = 0x25;
static const uint8_t CMD_WRITE = 0x30;
static const uint8_t CMD_WRITE_EXT = 0x34;
static const uint8_t CMD_WRITE_DMA = 0xCA;
static const uint8_t CMD_WRITE_DMA_EXT = 0x35;
static const uint8_t CMD_FLUSH_CACHE = 0xE7;
static const uint8_t CMD_FLUSH_CACHE_EXT = 0xEA;
static const uint8_t CMD_IDENTIFY = 0xEC;

static const uint8_t STATUS_ERROR = 1 << 0;
static const uint8_t STATUS_DATAREADY = 1 << 3;
static const uint8_t STATUS_DRIVEFAULT = 1 << 5;
static const uint8_t STATUS_BUSY = 1 << 7;

static const uint16_t BUSMASTER_REG_COMMAND = 0x0;
static const uint16_t BUSMASTER_REG_STATUS = 0x2;
static const uint16_t BUSMASTER_REG_PDRT = 0x4;

static const uint16_t BUSMASTER_COMMAND_START = 1 << 0;
static const uint16_t BUSMASTER_COMMAND_READING = 1 << 3;

static const uint8_t BUSMASTER_STATUS_DMA = 1 << 0;
static const uint8_t BUSMASTER_STATUS_DMA_FAILURE = 1 << 1;
static const uint8_t BUSMASTER_STATUS_INTERRUPT_PENDING = 1 << 2;
static const uint8_t BUSMASTER_STATUS_MASTER_DMA_INIT = 1 << 5;
static const uint8_t BUSMASTER_STATUS_SLAVE_DMA_INIT = 1 << 6;
static const uint8_t BUSMASTER_STATUS_SIMPLEX = 1 << 7;

} // namespace ATA
} // namespace Sortix

#endif
