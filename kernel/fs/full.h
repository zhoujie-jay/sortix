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

    fs/full.h
    Bit bucket special device.

*******************************************************************************/

#ifndef SORTIX_FS_FULL_H
#define SORTIX_FS_FULL_H

#include <sortix/kernel/inode.h>

namespace Sortix {

class Full : public AbstractInode
{
public:
	Full(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode);
	virtual ~Full();
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);
};

} // namespace Sortix

#endif
