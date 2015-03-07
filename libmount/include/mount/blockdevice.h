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

    mount/blockdevice.h
    Block device abstraction.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_BLOCKDEVICE_H
#define INCLUDE_MOUNT_BLOCKDEVICE_H

#include <sys/stat.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>

#include <mount/error.h>

struct filesystem;
struct harddisk;
struct partition;
struct partition_table;

struct blockdevice
{
	struct harddisk* hd;
	struct partition* p;
	struct partition_table* pt;
	struct filesystem* fs;
	enum partition_error pt_error;
	enum filesystem_error fs_error;
	void* user_ctx;
};

#if defined(__cplusplus)
extern "C" {
#endif

blksize_t blockdevice_logical_block_size(const struct blockdevice*);
bool blockdevice_check_reasonable_block_size(blksize_t);
off_t blockdevice_size(const struct blockdevice*);
size_t blockdevice_preadall(const struct blockdevice*, void*, size_t, off_t);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
