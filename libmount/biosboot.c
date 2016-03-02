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
 * biosboot.c
 * GPT bios boot partition.
 */

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
