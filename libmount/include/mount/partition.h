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

    mount/partition.h
    Partition abstraction.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_PARTITION_H
#define INCLUDE_MOUNT_PARTITION_H

#include <sys/types.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

#include <mount/blockdevice.h>
#include <mount/error.h>

enum partition_type
{
	PARTITION_TYPE_PRIMARY,
	PARTITION_TYPE_EXTENDED,
	PARTITION_TYPE_LOGICAL,
};

enum partition_table_type
{
	PARTITION_TABLE_TYPE_NONE,
	PARTITION_TABLE_TYPE_UNKNOWN,
	PARTITION_TABLE_TYPE_MBR,
	PARTITION_TABLE_TYPE_GPT,
};

struct partition
{
	struct blockdevice bdev;
	struct blockdevice* parent_bdev;
	char* path;
	off_t start;
	off_t length;
	off_t extended_start;
	off_t extended_length;
	enum partition_type type;
	enum partition_table_type table_type;
	unsigned int index;
	unsigned char gpt_type_guid[16];
	unsigned char gpt_unique_guid[16];
	unsigned char mbr_system_id;
	char* gpt_name;
	uint64_t gpt_attributes;
};

struct partition_table
{
	enum partition_table_type type;
	struct partition** partitions;
	size_t partitions_count;
	enum partition_error error;
	unsigned char gpt_disk_guid[16];
	void* raw_partition_table;
	off_t usable_start;
	off_t usable_end;
};

#if defined(__cplusplus)
extern "C" {
#endif

int partition_compare_index(const struct partition*, const struct partition*);
int partition_compare_index_indirect(const void*, const void*);
int partition_compare_start(const struct partition*, const struct partition*);
int partition_compare_start_indirect(const void*, const void*);
const char* partition_error_string(enum partition_error);
bool partition_check_overlap(const struct partition*, off_t, off_t);
void partition_release(struct partition*);
bool blockdevice_probe_partition_table_type(enum partition_table_type*, struct blockdevice*);
enum partition_error
blockdevice_get_partition_table(struct partition_table**, struct blockdevice*);
enum partition_error
blockdevice_get_partition_table_mbr(struct partition_table**, struct blockdevice*);
enum partition_error
blockdevice_get_partition_table_gpt(struct partition_table**, struct blockdevice*);
void partition_table_release(struct partition_table*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
