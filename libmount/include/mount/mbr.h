/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of Sortix libmount.

    Sortix libmount is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libmount is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libmount. If not, see <http://www.gnu.org/licenses/>.

    mount/mbr.h
    Master Boot Record.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_MBR_H
#define INCLUDE_MOUNT_MBR_H

#include <sys/types.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

struct blockdevice;
struct partition_table;

struct mbr_partition
{
	uint8_t flags;
	uint8_t start_head;
	uint16_t start_sector_cylinder;
	uint8_t system_id;
	uint8_t end_head;
	uint16_t end_sector_cylinder;
	uint32_t start_sector;
	uint32_t total_sectors;
};

static_assert(sizeof(struct mbr_partition) == 16, "sizeof(struct mbr_partition) == 16");

struct mbr
{
	uint8_t bootstrap[446];
	unsigned char partitions[4][16];
	uint8_t signature[2];
};

static_assert(sizeof(struct mbr) == 512, "sizeof(struct mbr) == 512");

struct mbr_ebr_link
{
	struct mbr ebr;
	off_t offset;
};

struct mbr_partition_table
{
	struct mbr mbr;
	size_t ebr_chain_count;
	struct mbr_ebr_link* ebr_chain;
};

#if defined(__cplusplus)
extern "C" {
#endif

bool mbr_is_partition_used(const struct mbr_partition* partition);
void mbr_partition_decode(struct mbr_partition*);
void mbr_partition_encode(struct mbr_partition*);
void mbr_partition_table_release(struct mbr_partition_table*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
