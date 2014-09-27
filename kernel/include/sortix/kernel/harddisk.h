/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    sortix/kernel/harddisk.h
    Harddisk interface.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_HARDDISK_H
#define INCLUDE_SORTIX_KERNEL_HARDDISK_H

#include <sys/types.h>

#include <sortix/kernel/ioctx.h>

namespace Sortix {

class Harddisk
{
public:
	virtual ~Harddisk() { }
	virtual off_t GetSize() = 0;
	virtual blkcnt_t GetBlockCount() = 0;
	virtual blksize_t GetBlockSize() = 0;
	virtual uint16_t GetCylinderCount() = 0;
	virtual uint16_t GetHeadCount() = 0;
	virtual uint16_t GetSectorCount() = 0;
	virtual const char* GetDriver() = 0;
	virtual const char* GetModel() = 0;
	virtual const char* GetSerial() = 0;
	virtual const char* GetRevision() = 0;
	virtual const unsigned char* GetATAIdentify(size_t* size_ptr) = 0;
	virtual int sync(ioctx_t* ctx) = 0;
	virtual ssize_t pread(ioctx_t* ctx, unsigned char* buf, size_t count, off_t off) = 0;
	virtual ssize_t pwrite(ioctx_t* ctx, const unsigned char* buf, size_t count, off_t off) = 0;

};

} // namespace Sortix

#endif
