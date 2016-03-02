/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * fs/random.cpp
 * Random device.
 */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sortix/stat.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>

#include "random.h"

namespace Sortix {

DevRandom::DevRandom(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
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

DevRandom::~DevRandom()
{
}

int DevRandom::truncate(ioctx_t* /*ctx*/, off_t /*length*/)
{
	return 0;
}

off_t DevRandom::lseek(ioctx_t* /*ctx*/, off_t offset, int /*whence*/)
{
	return offset;
}

ssize_t DevRandom::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	size_t sofar = 0;
	while ( count )
	{
		unsigned char buffer[512];
		size_t amount = count;
		if ( sizeof(buffer) < amount )
			amount = sizeof(buffer);
		arc4random_buf(buffer, amount);
		if ( !ctx->copy_to_dest(buf, buffer, amount) )
			return sofar ? sofar : -1;
		buf += amount;
		count -= amount;
		sofar += amount;
	}
	return (ssize_t) sofar;
}

ssize_t DevRandom::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t /*off*/)
{
	return read(ctx, buf, count);
}

ssize_t DevRandom::write(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count)
{
	return count;
}

ssize_t DevRandom::pwrite(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t count,
                    off_t /*off*/)
{
	return count;
}

} // namespace Sortix
