/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015, 2016.

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

    disk/node.cpp
    Inode adapter for harddisks.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sortix/stat.h>

#include <sortix/kernel/harddisk.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>

#include "node.h"

namespace Sortix {

PortNode::PortNode(Harddisk* harddisk, uid_t owner, gid_t group, mode_t mode,
                   dev_t dev, ino_t /*ino*/)
{
	this->harddisk = harddisk;
	inode_type = INODE_TYPE_FILE;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFBLK;
	this->stat_size = harddisk->GetSize();
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_blksize = harddisk->GetBlockSize();
	this->stat_blocks = harddisk->GetBlockCount();
}

PortNode::~PortNode()
{
	// TODO: Ownership of `port'.
}

int PortNode::sync(ioctx_t* ctx)
{
	return harddisk->sync(ctx);
}

int PortNode::truncate(ioctx_t* /*ctx*/, off_t length)
{
	if ( length != harddisk->GetSize() )
		return errno = EPERM, -1;
	return 0;
}

off_t PortNode::lseek(ioctx_t* /*ctx*/, off_t offset, int whence)
{
	if ( whence == SEEK_SET )
		return offset;
	if ( whence == SEEK_END )
	{
		off_t result;
		if ( __builtin_add_overflow(harddisk->GetSize(), offset, &result) )
			return errno = EOVERFLOW, -1;
		return result;
	}
	return errno = EINVAL, -1;
}

ssize_t PortNode::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	return harddisk->pread(ctx, (unsigned char*) buf, count, off);
}

ssize_t PortNode::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
                        off_t off)
{
	return harddisk->pwrite(ctx, (const unsigned char*) buf, count, off);
}

ssize_t PortNode::tcgetblob(ioctx_t* ctx, const char* name, void* buffer,
                            size_t count)
{
	const void* result_pointer = NULL;
	size_t result_size = 0;
	char string[sizeof(uintmax_t) * 3];
	static const char index[] = "harddisk-driver\0"
	                            "harddisk-model\0"
	                            "harddisk-serial\0"
	                            "harddisk-revision\0"
	                            "harddisk-size\0"
	                            "harddisk-block-count\0"
	                            "harddisk-block-size\0"
	                            "harddisk-cylinders\0"
	                            "harddisk-heads\0"
	                            "harddisk-sectors\0"
	                            "harddisk-ata-identify\0";

	if ( !name )
	{
		result_pointer = index;
		result_size = sizeof(index) - 1;
	}
	else if ( !strcmp(name, "harddisk-driver") )
	{
		result_pointer = harddisk->GetDriver();
		result_size = strlen((const char*) result_pointer);
	}
	else if ( !strcmp(name, "harddisk-model") )
	{
		result_pointer = harddisk->GetModel();
		result_size = strlen((const char*) result_pointer);
	}
	else if ( !strcmp(name, "harddisk-serial") )
	{
		result_pointer = harddisk->GetSerial();
		result_size = strlen((const char*) result_pointer);
	}
	else if ( !strcmp(name, "harddisk-revision") )
	{
		result_pointer = harddisk->GetRevision();
		result_size = strlen((const char*) result_pointer);
	}
	else if ( !strcmp(name, "harddisk-size") )
	{
		snprintf(string, sizeof(string), "%" PRIiOFF, harddisk->GetSize());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-block-count") )
	{
		snprintf(string, sizeof(string), "%" PRIiBLKCNT, harddisk->GetBlockCount());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-block-size") )
	{
		snprintf(string, sizeof(string), "%" PRIiBLKSIZE, harddisk->GetBlockSize());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-cylinders") )
	{
		snprintf(string, sizeof(string), "%" PRIu16, harddisk->GetCylinderCount());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-heads") )
	{
		snprintf(string, sizeof(string), "%" PRIu16, harddisk->GetHeadCount());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-sectors") )
	{
		snprintf(string, sizeof(string), "%" PRIu16, harddisk->GetSectorCount());
		result_pointer = string;
		result_size = strlen(string);
	}
	else if ( !strcmp(name, "harddisk-ata-identify") )
	{
		if ( !(result_pointer = harddisk->GetATAIdentify(&result_size)) )
			return -1;
	}
	else
	{
		return errno = ENOENT, -1;
	}

	if ( SSIZE_MAX < result_size )
		return errno = EOVERFLOW, -1;
	if ( buffer && count < result_size )
		return errno = ERANGE, -1;
	if ( buffer && !ctx->copy_to_dest(buffer, result_pointer, result_size) )
		return -1;
	return (ssize_t) result_size;
}

} // namespace Sortix
