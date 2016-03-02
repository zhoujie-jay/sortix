/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/fcache.h
 * Cache mechanism for file contents.
 */

#ifndef INCLUDE_SORTIX_KERNEL_FCACHE_H
#define INCLUDE_SORTIX_KERNEL_FCACHE_H

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>

namespace Sortix {

struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;

class BlockCache;
struct BlockCacheArea;
struct BlockCacheBlock;
class FileCache;
//class FileCacheBackend;

const uintptr_t BCACHE_PRESENT = 1 << 0;
const uintptr_t BCACHE_USED = 1 << 1;
const uintptr_t BCACHE_MODIFIED = 1 << 2;

class BlockCache
{
public:
	BlockCache();
	~BlockCache();
	BlockCacheBlock* AcquireBlock();
	void ReleaseBlock(BlockCacheBlock* block);
	void MarkUsed(BlockCacheBlock* block);
	void MarkModified(BlockCacheBlock* block);
	uint8_t* BlockData(BlockCacheBlock* block);

public:
	bool AddArea();
	void UnlinkBlock(BlockCacheBlock* block);
	void LinkBlock(BlockCacheBlock* block);
	uint8_t* BlockDataUnlocked(BlockCacheBlock* block);

private:
	BlockCacheArea* areas;
	size_t areas_used;
	size_t areas_length;
	size_t blocks_per_area;
	size_t unused_block_count;
	size_t blocks_used;
	size_t blocks_allocated;
	BlockCacheBlock* mru_block;
	BlockCacheBlock* lru_block;
	BlockCacheBlock* unused_block;
	kthread_mutex_t bcache_mutex;

};

struct BlockCacheArea
{
	uint8_t* data;
	BlockCacheBlock* blocks;
	struct addralloc_t addralloc;

};

struct BlockCacheBlock
{
	uintptr_t information;
	FileCache* fcache;
	BlockCacheBlock* next_block;
	BlockCacheBlock* prev_block;
	uintptr_t BlockId() { return information >> 12; }
};

class FileCache
{
public:
	static void Init();

public:
	FileCache(/*FileCacheBackend* backend = NULL*/);
	~FileCache();
	int sync(ioctx_t* ctx);
	ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off);
	int truncate(ioctx_t* ctx, off_t length);
	off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	//bool ChangeBackend(FileCacheBackend* backend, bool sync_old);
	off_t GetFileSize();

private:
	bool ChangeSize(off_t newsize, bool exact);
	bool ChangeNumBlocks(size_t numblocks, bool exact);
	bool Synchronize();
	void InitializeFileData(off_t to_where);

private:
	off_t file_size;
	off_t file_written;
	BlockCacheBlock** blocks;
	size_t blocks_used;
	size_t blocks_length;
	kthread_mutex_t fcache_mutex;
	bool modified;
	bool modified_size;
	//FileCacheBackend* backend;

};

/*
class FileCacheBackend
{
public:
	virtual ~FileCacheBackend() { }
	virtual int fcache_sync(ioctx_t* ctx) = 0;
	virtual ssize_t fcache_pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off) = 0;
	virtual ssize_t fcache_pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off) = 0;
	virtual int fcache_truncate(ioctx_t* ctx, off_t length) = 0;
	virtual off_t fcache_lseek(ioctx_t* ctx, off_t offset, int whence) = 0;

};
*/

} // namespace Sortix

#endif
