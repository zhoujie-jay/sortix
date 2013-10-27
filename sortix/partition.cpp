/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    partition.cpp
    Block device representing a partition of another block device.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>

#include <sortix/seek.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/refcount.h>

#include "partition.h"

namespace Sortix {

Partition::Partition(Ref<Inode> inner_inode, off_t start, off_t length)
{
	this->dev = (dev_t) this;
	this->ino = (ino_t) this;
	this->inode_type = INODE_TYPE_FILE;
	this->inner_inode = inner_inode;
	this->start = start;
	this->length = length;
	this->stat_size = length;
}

Partition::~Partition()
{
}

int Partition::sync(ioctx_t* ctx)
{
	return inner_inode->sync(ctx);
}

int Partition::truncate(ioctx_t* /*ctx*/, off_t new_length)
{
	if ( new_length != length )
		return errno = EPERM, -1;
	return 0;
}

off_t Partition::lseek(ioctx_t* /*ctx*/, off_t offset, int whence)
{
	if ( whence == SEEK_SET )
		return offset;
	if ( whence == SEEK_END )
		// TODO: Avoid underflow and overflow!
		return length + offset;
	return errno = EINVAL, -1;
}

ssize_t Partition::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	// TODO: Avoid underflow and overflow!
	off_t end_at = off + count;
	if ( length <= end_at )
		return 0;
	off_t available = length - off;
	if ( (uintmax_t) available < (uintmax_t) count )
		count = available;
	return inner_inode->pread(ctx, buf, count, start + off);
}

ssize_t Partition::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
                          off_t off)
{
	// TODO: Avoid underflow and overflow!
	off_t end_at = off + count;
	if ( length <= end_at )
		return 0;
	off_t available = length - off;
	if ( (uintmax_t) available < (uintmax_t) count )
		count = available;
	return inner_inode->pwrite(ctx, buf, count, start + off);
}

}
