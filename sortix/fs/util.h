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

    fs/util.h
    Utility classes for kernel filesystems.

*******************************************************************************/

#ifndef SORTIX_FS_UTIL_H
#define SORTIX_FS_UTIL_H

#include <sortix/kernel/inode.h>

namespace Sortix {

class UtilMemoryBuffer : public AbstractInode
{
public:
	UtilMemoryBuffer(dev_t dev, ino_t ino, uid_t owner, gid_t group,
	                 mode_t mode, uint8_t* buf, size_t bufsize,
	                 bool write = true, bool deletebuf = true);
	virtual ~UtilMemoryBuffer();
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);

private:
	kthread_mutex_t filelock;
	uint8_t* buf;
	size_t bufsize;
	bool write;
	bool deletebuf;

};

} // namespace Sortix

#endif
