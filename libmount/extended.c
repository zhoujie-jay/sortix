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
 * extended.c
 * MBR extended partition.
 */

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
