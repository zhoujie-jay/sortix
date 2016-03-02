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
 * sortix/kernel/harddisk.h
 * Harddisk interface.
 */

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
