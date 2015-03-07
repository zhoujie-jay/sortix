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

    extended.c
    MBR extended partition.

*******************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <mount/blockdevice.h>
#include <mount/extended.h>
#include <mount/filesystem.h>
#include <mount/partition.h>
#include <mount/uuid.h>

#include "util.h"

static size_t extended_probe_amount(struct blockdevice* bdev)
{
	(void) bdev;
	return 0;
}

static bool extended_probe(struct blockdevice* bdev,
                           const unsigned char* leading,
                           size_t amount)
{
	(void) leading;
	(void) amount;
	struct partition* p = bdev->p;
	if ( !p )
		return false;
	return p->table_type == PARTITION_TABLE_TYPE_MBR &&
		   (p->mbr_system_id == 0x05 ||
	        p->mbr_system_id == 0x0F);
}

static void extended_release(struct filesystem* fs)
{
	if ( !fs )
		return;
	free(fs);
}

static enum filesystem_error extended_inspect(struct filesystem** fs_ptr,
                                              struct blockdevice* bdev)
{
	*fs_ptr = NULL;
	struct filesystem* fs = CALLOC_TYPE(struct filesystem);
	if ( !fs )
		return FILESYSTEM_ERROR_ERRNO;
	fs->bdev = bdev;
	fs->handler = &extended_handler;
	fs->handler_private = NULL;
	fs->fstype_name = "extended";
	return *fs_ptr = fs, FILESYSTEM_ERROR_NONE;
}

const struct filesystem_handler extended_handler =
{
	.handler_name = "extended",
	.probe_amount = extended_probe_amount,
	.probe = extended_probe,
	.inspect = extended_inspect,
	.release = extended_release,
};
