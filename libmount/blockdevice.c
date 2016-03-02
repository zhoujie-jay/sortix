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
 * blockdevice.c
 * Block device abstraction.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <ioleast.h>
#include <stdbool.h>
#include <stdint.h>

#include <mount/blockdevice.h>
#include <mount/harddisk.h>
#include <mount/partition.h>

blksize_t blockdevice_logical_block_size(const struct blockdevice* bdev)
{
	while ( bdev->p )
		bdev = bdev->p->parent_bdev;
	assert(bdev->hd);
	return bdev->hd->logical_block_size;
}

bool blockdevice_check_reasonable_block_size(blksize_t size)
{
	if ( size < 512 ) // Violates assumptions.
		return false;
	for ( blksize_t cmp = 512; cmp <= 65536; cmp *= 2 )
		if ( cmp == size )
			return true;
	return false;
}

off_t blockdevice_size(const struct blockdevice* bdev)
{
	if ( bdev->p )
		return bdev->p->length;
	assert(bdev->hd);
	return bdev->hd->st.st_size;
}

size_t blockdevice_preadall(const struct blockdevice* bdev,
                            void* buffer,
                            size_t count,
                            off_t off)
{
	if ( off < 0 )
		return errno = EINVAL, 0;
	while ( bdev->p )
	{
		if ( bdev->p->length < off )
			return 0;
		off_t available = bdev->p->length - off;
		if ( (uintmax_t) available < (uintmax_t) count )
			count = (size_t) available;
		if ( __builtin_add_overflow(bdev->p->start, off, &off) )
			return 0;
		bdev = bdev->p->parent_bdev;
	}
	assert(bdev->hd);
	return preadall(bdev->hd->fd, buffer, count, off);
}
