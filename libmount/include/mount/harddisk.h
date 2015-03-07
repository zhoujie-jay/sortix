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

    mount/harddisk.h
    Harddisk abstraction.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_HARDDISK_H
#define INCLUDE_MOUNT_HARDDISK_H

#include <sys/stat.h>
#include <sys/types.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stdint.h>

#include <mount/blockdevice.h>

struct harddisk
{
	struct blockdevice bdev;
	struct stat st;
	int fd;
	char* path;
	char* driver;
	char* model;
	char* serial;
	blksize_t logical_block_size;
	uint16_t cylinders;
	uint16_t heads;
	uint16_t sectors;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct harddisk* harddisk_openat(int, const char*, int);
bool harddisk_inspect_blockdevice(struct harddisk*);
void harddisk_close(struct harddisk*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
