/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * fs/full.cpp
 * Bit bucket special device.
 */

#include <sys/types.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sortix/stat.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>

#include "full.h"

namespace Sortix {

Full::Full(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
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

Full::~Full()
{
}

int Full::truncate(ioctx_t* /*ctx*/, off_t /*length*/)
{
	return 0;
}

off_t Full::lseek(ioctx_t* /*ctx*/, off_t offset, int /*whence*/)
{
	return offset;
}

ssize_t Full::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	ctx->zero_dest(buf, count);
	return (ssize_t) count;
}

ssize_t Full::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t /*off*/)
{
	return read(ctx, buf, count);
}

ssize_t Full::write(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count)
{
	return count ? (errno = ENOSPC, -1) : 0;
}

ssize_t Full::pwrite(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count,
                    off_t /*off*/)
{
	return count ? (errno = ENOSPC, -1) : 0;
}

} // namespace Sortix
