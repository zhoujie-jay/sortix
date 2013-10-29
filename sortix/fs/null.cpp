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

    fs/null.cpp
    Bit bucket special device.

*******************************************************************************/

#include <sys/types.h>

#include <stdint.h>

#include <sortix/stat.h>

#include <sortix/kernel/inode.h>

#include "null.h"

namespace Sortix {

Null::Null(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
{
	inode_type = INODE_TYPE_STREAM;
	if ( !dev )
		dev = (dev_t) this;
	if ( !ino )
		ino = (ino_t) this;
	this->type = S_IFCHR;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_size = 0;
	this->stat_blksize = 1;
	this->dev = dev;
	this->ino = ino;
}

Null::~Null()
{
}

int Null::truncate(ioctx_t* /*ctx*/, off_t /*length*/)
{
	return 0;
}

off_t Null::lseek(ioctx_t* /*ctx*/, off_t offset, int /*whence*/)
{
	return offset;
}

ssize_t Null::read(ioctx_t* /*ctx*/, uint8_t* /*buf*/, size_t /*count*/)
{
	return 0;
}

ssize_t Null::pread(ioctx_t* /*ctx*/, uint8_t* /*buf*/, size_t /*count*/,
                    off_t /*off*/)
{
	return 0;
}

ssize_t Null::write(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count)
{
	return count;
}

ssize_t Null::pwrite(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count,
                    off_t /*off*/)
{
	return count;
}

} // namespace Sortix
