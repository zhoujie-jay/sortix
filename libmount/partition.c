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
 * partition.c
 * Partition abstraction.
 */

#include <sys/types.h>

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mount/blockdevice.h>
#include <mount/gpt.h>
#include <mount/mbr.h>
#include <mount/partition.h>

#include "util.h"

int partition_compare_index(const struct partition* a, const struct partition* b)
{
	if ( a->index < b->index )
		return -1;
	if ( b->index < a->index )
		return 1;
	return 0;
}

int partition_compare_index_indirect(const void* a_ptr, const void* b_ptr)
{
	const struct partition* a = *(const struct partition* const*) a_ptr;
	const struct partition* b = *(const struct partition* const*) b_ptr;
	return partition_compare_index(a, b);
}

int partition_compare_start(const struct partition* a, const struct partition* b)
{
	if ( a->start < b->start )
		return -1;
	if ( b->start < a->start )
		return 1;
	return 0;
}

int partition_compare_start_indirect(const void* a_ptr, const void* b_ptr)
{
	const struct partition* a = *(const struct partition* const*) a_ptr;
	const struct partition* b = *(const struct partition* const*) b_ptr;
	return partition_compare_index(a, b);
}

const char* partition_error_string(enum partition_error error)
{
	switch ( error )
	{
	case PARTITION_ERROR_NONE:
		break;
	case PARTITION_ERROR_ABSENT:
		return "No partition table found";
	case PARTITION_ERROR_UNRECOGNIZED:
		return "Unrecognized partitioning scheme";
	case PARTITION_ERROR_ERRNO:
		return (const char*) strerror(errno);
	case PARTITION_ERROR_INVALID:
		return "Invalid partition table";
	case PARTITION_ERROR_HEADER_TOO_LARGE:
		return "Partition table header larger than logical sector size";
	case PARTITION_ERROR_CHECKSUM:
		return "Partition table does not match its checksum";
	case PARTITION_ERROR_OVERLAP:
		return "Partition table contains overlapping partitions";
	case PARTITION_ERROR_END_BEFORE_START:
		return "Invalid partition ends before its start";
	case PARTITION_ERROR_BEFORE_USABLE:
		return "Invalid partition begins before the usable region";
	case PARTITION_ERROR_BEYOND_DEVICE:
		return "Invalid partition exceeds device";
	case PARTITION_ERROR_BEYOND_EXTENDED:
		return "Invalid logical partition exceeds its extended partition";
	case PARTITION_ERROR_BEYOND_USABLE:
		return "Invalid partition ends after the usable region";
	case PARTITION_ERROR_TOO_EXTENDED:
		return "Bad partition table (more than one extended partition)";
	case PARTITION_ERROR_BAD_EXTENDED:
		return "Bad extended partition";
	case PARTITION_ERROR_NONLINEAR_EXTENDED:
		return "Extended partition is not linearly linked together";
	}
	return "Unknown error condition";
}

bool partition_check_overlap(const struct partition* partition,
                             off_t start,
                             off_t length)
{
	assert(0 <= start);
	assert(0 <= length);
	assert(0 <= partition->start);
	assert(0 <= partition->length);
	if ( start <= partition->start )
	{
		off_t max_length = partition->start - start;
		return max_length < length;
	}
	else
	{
		off_t max_length = start - partition->start;
		return max_length < partition->length;
	}
}

bool blockdevice_probe_partition_table_type(enum partition_table_type* result_out,
                                            struct blockdevice* bdev)
{
	blksize_t logical_block_size = blockdevice_logical_block_size(bdev);
	if ( !blockdevice_check_reasonable_block_size(logical_block_size) )
		return errno = EINVAL, false;

	// TODO: Overflow checks for truncation, and the multiplication.
	size_t block_size = logical_block_size;
	size_t leading_size = 2 * block_size;
	unsigned char* leading = (unsigned char*) malloc(leading_size);
	if ( !leading )
		return false;

	size_t amount = blockdevice_preadall(bdev, leading, leading_size, 0);
	if ( amount < leading_size && errno != EEOF )
		return free(leading), false;

	enum partition_table_type result = PARTITION_TABLE_TYPE_NONE;

	do if ( 2*block_size <= amount )
	{
		struct gpt gpt;
		memcpy(&gpt, leading + 1 * block_size, sizeof(gpt));
		gpt_decode(&gpt);
		if ( memcmp(gpt.signature, "EFI PART", 8) != 0 )
			break;
		result = PARTITION_TABLE_TYPE_GPT;
		goto out;
	} while ( 0 );

	do if ( sizeof(struct mbr) <= amount )
	{
		struct mbr mbr;
		memcpy(&mbr, leading, sizeof(mbr));
		if ( mbr.signature[0] != 0x55 && mbr.signature[1] != 0xAA )
			break;
		result = PARTITION_TABLE_TYPE_MBR;
		goto out;
	} while ( 0 );

	for ( size_t i = 0; i < leading_size; i++ )
	{
		if ( leading[i] == 0x00 )
			continue;
		result = PARTITION_TABLE_TYPE_UNKNOWN;
		goto out;
	}

out:
	free(leading);
	return *result_out = result, true;
}

enum partition_error
blockdevice_get_partition_table(struct partition_table** pt_ptr,
                                struct blockdevice* bdev)
{
	enum partition_table_type pt_type;
	if ( !blockdevice_probe_partition_table_type(&pt_type, bdev) )
		return *pt_ptr = NULL, PARTITION_ERROR_ERRNO;
	switch ( pt_type )
	{
	case PARTITION_TABLE_TYPE_NONE:
		return *pt_ptr = NULL, PARTITION_ERROR_ABSENT;
	case PARTITION_TABLE_TYPE_UNKNOWN:
		break;
	case PARTITION_TABLE_TYPE_MBR:
		return blockdevice_get_partition_table_mbr(pt_ptr, bdev);
	case PARTITION_TABLE_TYPE_GPT:
		return blockdevice_get_partition_table_gpt(pt_ptr, bdev);
	}
	return *pt_ptr = NULL, PARTITION_ERROR_UNRECOGNIZED;
}

void partition_release(struct partition* p)
{
	if ( !p )
		return;
	free(p->gpt_name);
	free(p);
}

void partition_table_release(struct partition_table* pt)
{
	if ( !pt )
		return;
	switch ( pt->type )
	{
	case PARTITION_TABLE_TYPE_NONE: break;
	case PARTITION_TABLE_TYPE_UNKNOWN: break;
	case PARTITION_TABLE_TYPE_MBR:
		mbr_partition_table_release(
			(struct mbr_partition_table*) pt->raw_partition_table);
		break;
	case PARTITION_TABLE_TYPE_GPT:
		gpt_partition_table_release(
			(struct gpt_partition_table*) pt->raw_partition_table);
		break;
	}
	for ( size_t i = 0; i < pt->partitions_count; i++ )
		partition_release(pt->partitions[i]);
	free(pt->partitions);
	free(pt);
}
