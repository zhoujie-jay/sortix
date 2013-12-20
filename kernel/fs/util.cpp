/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    fs/util.cpp
    Utility classes for kernel filesystems.

*******************************************************************************/

#include <errno.h>
#include <string.h>

#include <sortix/stat.h>
#include <sortix/seek.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

#include "util.h"

namespace Sortix {

UtilMemoryBuffer::UtilMemoryBuffer(dev_t dev, ino_t ino, uid_t owner,
                                   gid_t group, mode_t mode, uint8_t* buf,
                                   size_t bufsize, bool write, bool deletebuf)
{
	inode_type = INODE_TYPE_FILE;
	this->filelock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFREG;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_blksize = 1;
	this->stat_size = (off_t) bufsize;
	this->dev = dev;
	this->ino = ino ? ino : (ino_t) this;
	this->buf = buf;
	this->bufsize = bufsize;
	this->write = write;
	this->deletebuf = deletebuf;
}

UtilMemoryBuffer::~UtilMemoryBuffer()
{
	if ( deletebuf )
		delete[] buf;
}

int UtilMemoryBuffer::truncate(ioctx_t* /*ctx*/, off_t length)
{
	ScopedLock lock(&filelock);
	if ( (uintmax_t) length != (uintmax_t) bufsize )
		return errno = ENOTSUP, -1;
	return 0;
}

off_t UtilMemoryBuffer::lseek(ioctx_t* /*ctx*/, off_t offset, int whence)
{
	ScopedLock lock(&filelock);
	if ( whence == SEEK_SET )
		return offset;
	if ( whence == SEEK_END )
		return (off_t) bufsize + offset;
	errno = EINVAL;
	return -1;
}

ssize_t UtilMemoryBuffer::pread(ioctx_t* ctx, uint8_t* dest, size_t count,
                                off_t off)
{
	ScopedLock lock(&filelock);
	if ( (uintmax_t) bufsize < (uintmax_t) off )
		return 0;
	size_t available = bufsize - off;
	if ( available < count )
		count = available;
	if ( !ctx->copy_to_dest(dest, buf + off, count) )
		return -1;
	return count;
}

ssize_t UtilMemoryBuffer::pwrite(ioctx_t* ctx, const uint8_t* src, size_t count,
                                 off_t off)
{
	ScopedLock lock(&filelock);
	if ( !write ) { errno = EBADF; return -1; }
	// TODO: Avoid having off + count overflow!
	if ( bufsize < off + count )
		return 0;
	if ( (uintmax_t) bufsize <= (uintmax_t) off )
		return -1;
	size_t available = bufsize - off;
	if ( available < count )
		count = available;
	ctx->copy_from_src(buf + off, src, count);
	return count;
}

} // namespace Sortix
