/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    devices.h
    Utility functions to handle devices, partitions, and filesystems.

*******************************************************************************/

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
