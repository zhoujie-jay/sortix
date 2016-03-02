/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
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
 * devices.h
 * Utility functions to handle devices, partitions, and filesystems.
 */

#ifndef DEVICES_H
#define DEVICES_H

struct mountpoint
{
	struct fstab entry;
	char* entry_line;
	pid_t pid;
	char* absolute;
	struct filesystem* fs;
};

extern struct harddisk** hds;
extern size_t hds_count;

const char* path_of_blockdevice(struct blockdevice* bdev);
const char* device_path_of_blockdevice(struct blockdevice* bdev);
void unscan_filesystem(struct blockdevice* bdev);
void scan_filesystem(struct blockdevice* bdev);
void unscan_device(struct harddisk* hd);
void scan_device(struct harddisk* hd);
void unscan_devices(void);
void scan_devices(void);
struct filesystem* search_for_filesystem_by_uuid(const unsigned char* uuid);
struct filesystem* search_for_filesystem_by_spec(const char* spec);
bool check_existing_systems(void);
bool check_lacking_partition_table(void);
bool fsck(struct filesystem* fs);
void free_mountpoints(struct mountpoint* mnts, size_t mnts_count);
bool load_mountpoints(const char* fstab_path,
                      struct mountpoint** mountpoints_out,
                      size_t* mountpoints_used_out);
void mountpoint_mount(struct mountpoint* mountpoint);
void mountpoint_unmount(struct mountpoint* mountpoint);

#endif
