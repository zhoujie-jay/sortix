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

    biosboot.c
    GPT bios boot partition.

*******************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <mount/biosboot.h>
#include <mount/blockdevice.h>
#include <mount/filesystem.h>
#include <mount/partition.h>
#include <mount/uuid.h>

#include "util.h"

static size_t biosboot_probe_amount(struct blockdevice* bdev)
{
	(void) bdev;
	return 0;
}

static bool biosboot_probe(struct blockdevice* bdev,
                           const unsigned char* leading,
                           size_t amount)
{
	(void) leading;
	(void) amount;
	struct partition* p = bdev->p;
	if ( !p )
		return false;
	if ( p->table_type != PARTITION_TABLE_TYPE_GPT )
		return false;
	unsigned char uuid[16];
	uuid_from_string(uuid, BIOSBOOT_GPT_TYPE_UUID);
	return memcmp(p->gpt_type_guid, uuid, 16) == 0;
}

static void biosboot_release(struct filesystem* fs)
{
	if ( !fs )
		return;
	free(fs);
}

static enum filesystem_error biosboot_inspect(struct filesystem** fs_ptr,
                                              struct blockdevice* bdev)
{
	*fs_ptr = NULL;
	struct filesystem* fs = CALLOC_TYPE(struct filesystem);
	if ( !fs )
		return FILESYSTEM_ERROR_ERRNO;
	fs->bdev = bdev;
	fs->handler = &biosboot_handler;
	fs->handler_private = NULL;
	fs->fstype_name = "biosboot";
	return *fs_ptr = fs, FILESYSTEM_ERROR_NONE;
}

const struct filesystem_handler biosboot_handler =
{
	.handler_name = "biosboot",
	.probe_amount = biosboot_probe_amount,
	.probe = biosboot_probe,
	.inspect = biosboot_inspect,
	.release = biosboot_release,
};
