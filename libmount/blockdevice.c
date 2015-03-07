/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

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

    blockdevice.c
    Block device abstraction.

*******************************************************************************/

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
