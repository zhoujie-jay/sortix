/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * mount/mbr.h
 * Master Boot Record.
 */

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
