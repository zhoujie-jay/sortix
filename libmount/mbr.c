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
 * mbr.c
 * Master Boot Record.
 */

#include <sys/types.h>

#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mount/blockdevice.h>
#include <mount/harddisk.h>
#include <mount/mbr.h>
#include <mount/partition.h>

#include "util.h"

void mbr_partition_decode(struct mbr_partition* v)
{
	// flags is endian agnostic.
	// start_head is endian agnostic.
	v->start_sector_cylinder = le16toh(v->start_sector_cylinder);
	v->end_sector_cylinder = le16toh(v->end_sector_cylinder);
	// system_id is endian agnostic.
	// end_head is endian agnostic.
	v->start_sector = le32toh(v->start_sector);
	v->total_sectors = le32toh(v->total_sectors);
}

void mbr_partition_encode(struct mbr_partition* v)
{
	v->start_sector_cylinder = htole16(v->start_sector_cylinder);
	v->end_sector_cylinder = htole16(v->end_sector_cylinder);
	// flags is endian agnostic.
	// start_head is endian agnostic.
	v->start_sector = htole32(v->start_sector);
	v->total_sectors = htole32(v->total_sectors);
	// system_id is endian agnostic.
	// end_head is endian agnostic.
}

void mbr_partition_table_release(struct mbr_partition_table* pt)
{
	if ( !pt )
		return;
	free(pt->ebr_chain);
	free(pt);
}

static bool mbr_is_extended_partition(const struct mbr_partition* partition)
{
	return partition->system_id == 0x05 || // CHS addressing.
	       partition->system_id == 0x0F;   // LBA addressing.
}

bool mbr_is_partition_used(const struct mbr_partition* partition)
{
	if ( memiszero(partition, sizeof(*partition)) )
		return false;
	if ( !partition->system_id ||
	     !partition->total_sectors )
		return false;
	return true;
}

enum partition_error
blockdevice_get_partition_table_mbr(struct partition_table** pt_ptr,
                                    struct blockdevice* bdev)
{
	*pt_ptr = NULL;
	blksize_t logical_block_size = blockdevice_logical_block_size(bdev);
	if ( !blockdevice_check_reasonable_block_size(logical_block_size) )
		return errno = EINVAL, PARTITION_ERROR_ERRNO;
	off_t device_size = blockdevice_size(bdev);
	const char* device_path = bdev->p ? bdev->p->path : bdev->hd->path;

	struct mbr mbr;
	if ( blockdevice_preadall(bdev, &mbr, sizeof(mbr), 0) != sizeof(mbr) )
		return PARTITION_ERROR_ERRNO;

	if ( mbr.signature[0] != 0x55 && mbr.signature[1] != 0xAA )
		return errno = EINVAL, PARTITION_ERROR_ERRNO;

	struct partition_table* pt = CALLOC_TYPE(struct partition_table);
	if ( !pt )
		return PARTITION_ERROR_ERRNO;
	*pt_ptr = pt;
	pt->type = PARTITION_TABLE_TYPE_MBR;
	size_t pt__partitions_length = 0;
	pt->usable_start = logical_block_size;
	off_t last_sector = device_size / logical_block_size;
	if ( UINT32_MAX + 1LL < last_sector )
		last_sector = UINT32_MAX + 1LL;
	pt->usable_end = last_sector * logical_block_size;

	struct mbr_partition_table* mbrpt = CALLOC_TYPE(struct mbr_partition_table);
	if ( !mbrpt )
		return PARTITION_ERROR_ERRNO;
	pt->raw_partition_table = mbrpt;
	memcpy(&mbrpt->mbr, &mbr, sizeof(mbr));

	unsigned int extended_partition_count = 0;
	for ( unsigned int i = 0; i < 4; i++ )
	{
		struct mbr_partition pmbr;
		memcpy(&pmbr, mbr.partitions[i], sizeof(pmbr));
		mbr_partition_decode(&pmbr);
		if ( mbr_is_partition_used(&pmbr) && mbr_is_extended_partition(&pmbr) )
			extended_partition_count++;
	}

	if ( 2 <= extended_partition_count ) // Violates assumptions.
		return PARTITION_ERROR_TOO_EXTENDED;

	for ( unsigned int i = 0; i < 4; i++ )
	{
		struct mbr_partition pmbr;
		memcpy(&pmbr, mbr.partitions[i], sizeof(pmbr));
		mbr_partition_decode(&pmbr);
		if ( !mbr_is_partition_used(&pmbr) )
			continue;
		// TODO: Potential overflow.
		if ( pmbr.start_sector == 0 )
			return PARTITION_ERROR_BEFORE_USABLE;
		off_t start = (off_t) pmbr.start_sector * (off_t) logical_block_size;
		off_t length = (off_t) pmbr.total_sectors * (off_t) logical_block_size;
		if ( device_size < start || device_size - start < length )
			return PARTITION_ERROR_BEYOND_DEVICE;
		for ( size_t j = 0; j < pt->partitions_count; j++ )
			if ( partition_check_overlap(pt->partitions[j], start, length) )
				return PARTITION_ERROR_OVERLAP;
		struct partition* p = CALLOC_TYPE(struct partition);
		if ( !p )
			return PARTITION_ERROR_ERRNO;
		memset(&p->bdev, 0, sizeof(p->bdev));
		p->bdev.p = p;
		p->parent_bdev = bdev;
		p->index = 1 + i;
		p->start = start;
		p->length = length;
		p->type = PARTITION_TYPE_PRIMARY;
		if ( mbr_is_extended_partition(&pmbr) )
		{
			p->extended_start = start;
			p->extended_length = length;
			p->type = PARTITION_TYPE_EXTENDED;
		}
		p->table_type = PARTITION_TABLE_TYPE_MBR;
		p->mbr_system_id = pmbr.system_id;
		if ( !array_add((void***) &pt->partitions,
		                &pt->partitions_count,
		                &pt__partitions_length,
		                p) )
			return free(p), PARTITION_ERROR_ERRNO;
		if ( device_path &&
		     asprintf(&p->path, "%sp%u", device_path, p->index) < 0 )
			return PARTITION_ERROR_ERRNO;
	}

	for ( unsigned int i = 0; i < 4; i++ )
	{
		struct mbr_partition pextmbr;
		memcpy(&pextmbr, mbr.partitions[i], sizeof(pextmbr));
		mbr_partition_decode(&pextmbr);
		if ( !mbr_is_partition_used(&pextmbr) )
			continue;
		if ( !mbr_is_extended_partition(&pextmbr) )
			continue;
		// TODO: Potential overflow.
		off_t pext_start = (off_t) pextmbr.start_sector * (off_t) logical_block_size;
		off_t pext_length = (off_t) pextmbr.total_sectors * (off_t) logical_block_size;
		off_t ebr_rel = 0;
		unsigned int j = 0;
		while ( true )
		{
			struct mbr ebr;
			if ( pext_length <= ebr_rel )
				return PARTITION_ERROR_BEYOND_EXTENDED;
			off_t ebr_off = pext_start + ebr_rel;
			if ( blockdevice_preadall(bdev, &ebr, sizeof(ebr), ebr_off) != sizeof(ebr) )
				return PARTITION_ERROR_ERRNO;
			if ( ebr.signature[0] != 0x55 && ebr.signature[1] != 0xAA )
				return PARTITION_ERROR_BAD_EXTENDED;
			size_t new_chain_count = mbrpt->ebr_chain_count + 1;
			size_t chain_size = sizeof(struct mbr_ebr_link) * new_chain_count;
			struct mbr_ebr_link* new_chain =
				(struct mbr_ebr_link*) realloc(mbrpt->ebr_chain, chain_size);
			if ( !new_chain )
				return PARTITION_ERROR_ERRNO;
			mbrpt->ebr_chain = new_chain;
			mbrpt->ebr_chain_count = new_chain_count;
			memcpy(&mbrpt->ebr_chain[j].ebr, &ebr, sizeof(ebr));
			mbrpt->ebr_chain[j].offset = ebr_off;
			struct mbr_partition pmbr;
			memcpy(&pmbr, ebr.partitions[0], sizeof(pmbr));
			mbr_partition_decode(&pmbr);
			if ( mbr_is_partition_used(&pmbr) )
			{
				// TODO: Potential overflow.
				off_t start = (off_t) pmbr.start_sector * (off_t) logical_block_size;
				off_t length = (off_t) pmbr.total_sectors * (off_t) logical_block_size;
				if ( pext_length - ebr_rel < start )
					return PARTITION_ERROR_BEYOND_EXTENDED;
				off_t max_length = (pext_length - ebr_rel) - start;
				if ( max_length < length )
					return PARTITION_ERROR_BEYOND_EXTENDED;
				struct partition* p = CALLOC_TYPE(struct partition);
				if ( !p )
					return PARTITION_ERROR_ERRNO;
				memset(&p->bdev, 0, sizeof(p->bdev));
				p->bdev.p = p;
				p->parent_bdev = bdev;
				p->index = 5 + j;
				p->start = ebr_off + start;
				p->length = length;
				p->type = PARTITION_TYPE_LOGICAL;
				p->table_type = PARTITION_TABLE_TYPE_MBR;
				p->mbr_system_id = pmbr.system_id;
				if ( !array_add((void***) &pt->partitions,
						        &pt->partitions_count,
						        &pt__partitions_length,
						        p) )
					return free(p), PARTITION_ERROR_ERRNO;
				if ( device_path &&
				     asprintf(&p->path, "%sp%u", device_path, p->index) < 0 )
					return PARTITION_ERROR_ERRNO;
			}
			j++;
			struct mbr_partition next;
			memcpy(&next, ebr.partitions[1], sizeof(next));
			mbr_partition_decode(&next);
			// TODO: Potential overflow.
			off_t next_rel = (off_t) next.start_sector * (off_t) logical_block_size;
			if ( !next_rel )
				break;
			if ( next_rel <= ebr_rel ) // Violates assumptions.
				return PARTITION_ERROR_NONLINEAR_EXTENDED;
			if ( pext_length <= next_rel )
				return PARTITION_ERROR_BEYOND_EXTENDED;
			ebr_rel = next_rel;
		}
		break;
	}

	return PARTITION_ERROR_NONE;
}
