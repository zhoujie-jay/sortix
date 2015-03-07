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

    mount/filesystem.h
    Filesystem abstraction.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_FILESYSTEM_H
#define INCLUDE_MOUNT_FILESYSTEM_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <time.h>

#include <mount/error.h>

struct blockdevice;
struct filesystem;
struct filesystem_handler;

#define FILESYSTEM_FLAG_UUID (1 << 0)
#define FILESYSTEM_FLAG_FSCK_SHOULD (1 << 1)
#define FILESYSTEM_FLAG_FSCK_MUST (1 << 2)

struct filesystem
{
	struct blockdevice* bdev;
	const struct filesystem_handler* handler;
	void* handler_private;
	const char* fstype_name;
	const char* fsck;
	const char* driver;
	int flags;
	unsigned char uuid[16];
	void* user_ctx;
};

struct filesystem_handler
{
	const char* handler_name;
	size_t (*probe_amount)(struct blockdevice*);
	bool (*probe)(struct blockdevice*, const unsigned char*, size_t);
	enum filesystem_error (*inspect)(struct filesystem**, struct blockdevice*);
	void (*release)(struct filesystem*);
};

#if defined(__cplusplus)
extern "C" {
#endif

const char* filesystem_error_string(enum filesystem_error);
enum filesystem_error
blockdevice_inspect_filesystem(struct filesystem**, struct blockdevice*);
void filesystem_release(struct filesystem*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
