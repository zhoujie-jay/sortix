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
 * mount/gpt.h
 * GUID Partition Table.
 */

#ifndef INCLUDE_MOUNT_GPT_H
#define INCLUDE_MOUNT_GPT_H

#include <sys/types.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

struct blockdevice;
struct partition_table;

struct gpt
{
	char signature[8];
	uint32_t revision;
	uint32_t header_size;
	uint32_t header_crc32;
	uint32_t reserved0;
	uint64_t my_lba;
	uint64_t alternate_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;
	unsigned char disk_guid[16];
	uint64_t partition_entry_lba;
	uint32_t number_of_partition_entries;
	uint32_t size_of_partition_entry;
	uint32_t partition_entry_array_crc32;
	unsigned char reserved1[420];
};

static_assert(sizeof(struct gpt) == 512, "sizeof(struct gpt) == 512");

#define GPT_PARTITION_NAME_LENGTH 36
struct gpt_partition
{
	unsigned char partition_type_guid[16];
	unsigned char unique_partition_guid[16];
	uint64_t starting_lba;
	uint64_t ending_lba;
	uint64_t attributes;
	uint16_t partition_name[GPT_PARTITION_NAME_LENGTH];
};

static_assert(sizeof(struct gpt_partition) == 128, "sizeof(struct gpt_partition) == 128");

struct gpt_partition_table
{
	struct gpt gpt;
	unsigned char* rpt;
};

#if defined(__cplusplus)
extern "C" {
#endif

void gpt_decode(struct gpt*);
void gpt_encode(struct gpt*);
void gpt_partition_decode(struct gpt_partition*);
void gpt_partition_encode(struct gpt_partition*);
char* gpt_decode_utf16(uint16_t* string, size_t length);
uint32_t gpt_crc32(const void*, size_t);
void gpt_partition_table_release(struct gpt_partition_table*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
