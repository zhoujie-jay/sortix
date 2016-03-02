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
 * gpt.c
 * GUID Partition Table.
 */

#include <sys/types.h>

#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <mount/blockdevice.h>
#include <mount/gpt.h>
#include <mount/harddisk.h>
#include <mount/partition.h>

#include "util.h"

void gpt_decode(struct gpt* v)
{
	// signature is endian agnostic.
	v->revision = le32toh(v->revision);
	v->header_size = le32toh(v->header_size);
	v->header_crc32 = le32toh(v->header_crc32);
	v->reserved0 = le32toh(v->reserved0);
	v->my_lba = le64toh(v->my_lba);
	v->alternate_lba = le64toh(v->alternate_lba);
	v->first_usable_lba = le64toh(v->first_usable_lba);
	v->last_usable_lba = le64toh(v->last_usable_lba);
	// disk_guid is endian agnostic.
	v->partition_entry_lba = le64toh(v->partition_entry_lba);
	v->number_of_partition_entries = le32toh(v->number_of_partition_entries);
	v->size_of_partition_entry = le32toh(v->size_of_partition_entry);
	v->partition_entry_array_crc32 = le32toh(v->partition_entry_array_crc32);
	// reserved bytes are endian agnostic.
}

void gpt_encode(struct gpt* v)
{
	// signature is endian agnostic.
	v->revision = htole32(v->revision);
	v->header_size = htole32(v->header_size);
	v->header_crc32 = htole32(v->header_crc32);
	v->reserved0 = htole32(v->reserved0);
	v->my_lba = htole64(v->my_lba);
	v->alternate_lba = htole64(v->alternate_lba);
	v->first_usable_lba = htole64(v->first_usable_lba);
	v->last_usable_lba = htole64(v->last_usable_lba);
	// disk_guid is endian agnostic.
	v->partition_entry_lba = htole64(v->partition_entry_lba);
	v->number_of_partition_entries = htole32(v->number_of_partition_entries);
	v->size_of_partition_entry = htole32(v->size_of_partition_entry);
	v->partition_entry_array_crc32 = htole32(v->partition_entry_array_crc32);
	// reserved bytes are endian agnostic.
}

void gpt_partition_decode(struct gpt_partition* v)
{
	// partition_type_guid is endian agnostic.
	// unique_partition_guid is endian agnostic.
	v->starting_lba = le64toh(v->starting_lba);
	v->ending_lba = le64toh(v->ending_lba);
	v->attributes = le64toh(v->attributes);
	for ( size_t i = 0; i < GPT_PARTITION_NAME_LENGTH; i++ )
		v->partition_name[i] = le16toh(v->partition_name[i]);
}

void gpt_partition_encode(struct gpt_partition* v)
{
	// partition_type_guid is endian agnostic.
	// unique_partition_guid is endian agnostic.
	v->starting_lba = htole64(v->starting_lba);
	v->ending_lba = htole64(v->ending_lba);
	v->attributes = htole64(v->attributes);
	for ( size_t i = 0; i < GPT_PARTITION_NAME_LENGTH; i++ )
		v->partition_name[i] = htole16(v->partition_name[i]);
}

char* gpt_decode_utf16(uint16_t* string, size_t length)
{
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	char* result = NULL;
	size_t result_used = 0;
	size_t result_length = 0;

	for ( size_t i = 0; true; i++ )
	{
		wchar_t wc;
		if ( length <= i )
			wc = L'\0';
		else if ( 0xDC00 <= string[i] && string[i] <= 0xDFFF )
			return free(result), errno = EILSEQ, (char*) NULL;
		else if ( 0xD800 <= string[i] && string[i] <= 0xDBFF )
		{
			uint16_t high = string[i] - 0xD800;
			if ( i + 1 == length )
				return free(result), errno = EILSEQ, (char*) NULL;
			if ( !(0xDC00 <= string[i+1] && string[i] <= 0xDFFF) )
				return free(result), errno = EILSEQ, (char*) NULL;
			uint16_t low = string[++i] - 0xDC00;
			wc = (((wchar_t) high << 10) | ((wchar_t) low << 0)) + 0x10000;
		}
		else
			wc = (wchar_t) string[i++];
		char mb[MB_CUR_MAX];
		size_t amount = wcrtomb(mb, wc, &ps);
		if ( amount == (size_t) -1 )
			return free(result), (char*) NULL;
		for ( size_t n = 0; n < amount; n++ )
		{
			if ( result_used == result_length )
			{
				// TODO: Potential overflow.
				size_t new_length = result_length ? 2 * result_length : length;
				char* new_result = (char*) realloc(result, new_length);
				if ( !new_result )
					return free(result), (char*) NULL;
				result = new_result;
				result_length = new_length;
			}
			result[result_used++] = mb[n];
		}
		if ( wc == L'\0' )
		{
			char* new_result = (char*) realloc(result, result_used);
			if ( new_result )
				result = new_result;
			return result;
		}
	}
}

void gpt_partition_table_release(struct gpt_partition_table* pt)
{
	if ( !pt )
		return;
	free(pt->rpt);
	free(pt);
}

static bool gpt_check_power_of_two(uintmax_t value,
                                   uintmax_t low,
                                   uintmax_t high)
{
	for ( uintmax_t cmp = low; cmp <= high; cmp *= 2 )
		if ( value == cmp )
			return true;
	return false;
}

enum partition_error
blockdevice_get_partition_table_gpt(struct partition_table** pt_ptr,
                                    struct blockdevice* bdev)
{
	*pt_ptr = NULL;
	blksize_t logical_block_size = blockdevice_logical_block_size(bdev);
	if ( !blockdevice_check_reasonable_block_size(logical_block_size) )
		return errno = EINVAL, PARTITION_ERROR_ERRNO;
	off_t device_size = blockdevice_size(bdev);
	const char* device_path = bdev->p ? bdev->p->path : bdev->hd->path;

	unsigned char* gpt_block = (unsigned char*) malloc(logical_block_size);
	if ( !gpt_block )
		return PARTITION_ERROR_ERRNO;
	if ( blockdevice_preadall(bdev, gpt_block, logical_block_size,
	                          1 * logical_block_size) != (size_t) logical_block_size )
		return free(gpt_block), PARTITION_ERROR_ERRNO;

	struct gpt gpt;
	memcpy(&gpt, gpt_block, sizeof(gpt));
	gpt_decode(&gpt);

	if ( memcmp(gpt.signature, "EFI PART", 8) != 0 )
		return PARTITION_ERROR_INVALID;
	static_assert( 92 == sizeof(gpt) - sizeof(gpt.reserved1),
	              "92 == sizeof(gpt) - sizeof(gpt.reserved1)");
	if ( gpt.header_size < 92 )
		return free(gpt_block), PARTITION_ERROR_INVALID;

	struct partition_table* pt = CALLOC_TYPE(struct partition_table);
	if ( !pt )
		return free(gpt_block), PARTITION_ERROR_ERRNO;
	*pt_ptr = pt;
	pt->type = PARTITION_TABLE_TYPE_GPT;
	size_t pt__partitions_length = 0;

	if ( logical_block_size < gpt.header_size )
		return free(gpt_block), PARTITION_ERROR_HEADER_TOO_LARGE;

	struct gpt_partition_table* gptpt = CALLOC_TYPE(struct gpt_partition_table);
	if ( !gptpt )
		return free(gpt_block), PARTITION_ERROR_ERRNO;
	pt->raw_partition_table = gptpt;
	memcpy(&gptpt->gpt, gpt_block, sizeof(gptpt->gpt));

	struct gpt* check_gpt = (struct gpt*) gpt_block;
	check_gpt->header_crc32 = 0;
	uint32_t checksum = gpt_crc32(gpt_block, gpt.header_size);
	free(gpt_block);
	if ( checksum != gpt.header_crc32 )
		return PARTITION_ERROR_CHECKSUM;

	memcpy(pt->gpt_disk_guid, gpt.disk_guid, 16);

	if ( !gpt_check_power_of_two(gpt.size_of_partition_entry,
	                             sizeof(struct gpt_partition), 65536) )
		return PARTITION_ERROR_INVALID;
	if ( gpt.my_lba != 1 )
		return PARTITION_ERROR_INVALID;

	if ( gpt.my_lba >= gpt.first_usable_lba )
		return PARTITION_ERROR_INVALID;
	// TODO: Ensure nothing collides with anything else.
	if ( gpt.partition_entry_lba <= 1 )
		return PARTITION_ERROR_INVALID;
	if ( gpt.last_usable_lba < gpt.first_usable_lba )
		return PARTITION_ERROR_INVALID;

	// TODO: Potential overflow.
	pt->usable_start = (off_t) gpt.first_usable_lba * (off_t) logical_block_size;
	pt->usable_end = ((off_t) gpt.last_usable_lba + 1) * (off_t) logical_block_size;
	if ( device_size < pt->usable_end )
		return PARTITION_ERROR_INVALID;

	// TODO: Potential overflow.
	size_t rpt_size = (size_t) gpt.size_of_partition_entry *
	                  (size_t) gpt.number_of_partition_entries;
	off_t rpt_off = (off_t) gpt.partition_entry_lba * (off_t) logical_block_size;
	off_t rpt_end = rpt_off + rpt_off;
	if ( pt->usable_start < rpt_end )
		return PARTITION_ERROR_INVALID;
	if ( !(gptpt->rpt = (unsigned char*) malloc(rpt_size)) )
		return PARTITION_ERROR_ERRNO;
	unsigned char* rpt = gptpt->rpt;
	if ( blockdevice_preadall(bdev, rpt, rpt_size, rpt_off) != rpt_size )
		return PARTITION_ERROR_ERRNO;
	if ( gpt.partition_entry_array_crc32 != gpt_crc32(rpt, rpt_size) )
		return PARTITION_ERROR_CHECKSUM;

	for ( uint32_t i = 0; i < gpt.number_of_partition_entries; i++ )
	{
		size_t pentry_off = (size_t) i * (size_t) gpt.size_of_partition_entry;
		struct gpt_partition pentry;
		memcpy(&pentry, rpt + pentry_off, sizeof(pentry));
		gpt_partition_decode(&pentry);
		if ( memiszero(pentry.partition_type_guid, 16) )
			continue;
		if ( pentry.ending_lba < pentry.starting_lba )
			return PARTITION_ERROR_END_BEFORE_START;
		// TODO: Potential overflow.
		uint64_t lba_count = (pentry.ending_lba - pentry.starting_lba) + 1;
		off_t start = (off_t) pentry.starting_lba * (off_t) logical_block_size;
		if ( start < pt->usable_start )
			return PARTITION_ERROR_BEFORE_USABLE;
		off_t length = (off_t) lba_count * (off_t) logical_block_size;
		if ( pt->usable_end < start || pt->usable_end - start < length )
			return PARTITION_ERROR_BEYOND_USABLE;
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
		p->table_type = PARTITION_TABLE_TYPE_GPT;
		memcpy(p->gpt_type_guid, pentry.partition_type_guid, 16);
		memcpy(p->gpt_unique_guid, pentry.unique_partition_guid, 16);
		p->gpt_attributes = pentry.attributes;
		if ( !array_add((void***) &pt->partitions,
				        &pt->partitions_count,
				        &pt__partitions_length,
				        p) )
			return free(p), PARTITION_ERROR_ERRNO;
		if ( !(p->gpt_name = gpt_decode_utf16(pentry.partition_name,
		                                      GPT_PARTITION_NAME_LENGTH)) )
			return PARTITION_ERROR_ERRNO;
		if ( device_path &&
		     asprintf(&p->path, "%sp%u", device_path, p->index) < 0 )
			return PARTITION_ERROR_ERRNO;
	}

	qsort(pt->partitions, pt->partitions_count, sizeof(pt->partitions[0]),
	      partition_compare_start_indirect);
	for ( size_t i = 1; i < pt->partitions_count; i++ )
	{
		struct partition* a = pt->partitions[i-1];
		struct partition* b = pt->partitions[i];
		if ( partition_check_overlap(a, b->start, b->length) )
			return PARTITION_ERROR_OVERLAP;
	}
	qsort(pt->partitions, pt->partitions_count, sizeof(pt->partitions[0]),
	      partition_compare_index_indirect);

	return PARTITION_ERROR_NONE;
}
