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

    partition.h
    Block device representing a partition of another block device.

*******************************************************************************/

#ifndef PARTITION_H
#define PARTITION_H

#include <sortix/kernel/refcount.h>
#include <sortix/kernel/inode.h>

namespace Sortix {

class Partition : public AbstractInode
{
public:
	Partition(Ref<Inode> inner_inode, off_t start, off_t length);
	virtual ~Partition();

private:
	Ref<Inode> inner_inode;
	off_t start;
	off_t length;

public:
	virtual int sync(ioctx_t* ctx);
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);

};

} // namespace Sortix

#endif
