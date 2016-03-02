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
 * fcache.cpp
 * Cache mechanism for file contents.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/mman.h>
#include <sortix/seek.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/fcache.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>

namespace Sortix {

__attribute__((unused))
static uintptr_t MakeBlockId(size_t area, size_t blocks_per_area, size_t block)
{
	return (uintptr_t) area * (uintptr_t) blocks_per_area + (uintptr_t) block;
}

__attribute__((unused))
static uintptr_t MakeBlockInformation(uintptr_t blockid, uintptr_t flags)
{
	return (blockid << 12) | (flags & ((1 << (12+1)) - 1));
}

__attribute__((unused))
static uintptr_t FlagsOfBlockInformation(uintptr_t info)
{
	return info & ((1 << (12+1)) - 1);
}

__attribute__((unused))
static uintptr_t BlockIdOfBlockInformation(uintptr_t info)
{
	return info >> 12;
}

__attribute__((unused))
static size_t BlockOfBlockId(uintptr_t blockid, size_t blocks_per_area)
{
	return (size_t) (blockid % blocks_per_area);
}

__attribute__((unused))
static size_t AreaOfBlockId(uintptr_t blockid, size_t blocks_per_area)
{
	return (size_t) (blockid / blocks_per_area);
}

BlockCache::BlockCache()
{
	areas = NULL;
	areas_used = 0;
	areas_length = 0;
	blocks_per_area = 1024UL;
	unused_block_count = 0;
	blocks_used = 0;
	blocks_allocated = 0;
	mru_block = NULL;
	lru_block = NULL;
	unused_block = NULL;
	bcache_mutex = KTHREAD_MUTEX_INITIALIZER;
}

BlockCache::~BlockCache()
{
	// TODO: Clean up everything here!
}

BlockCacheBlock* BlockCache::AcquireBlock()
{
	ScopedLock lock(&bcache_mutex);
	if ( !unused_block && !AddArea() )
		return NULL;
	BlockCacheBlock* ret = unused_block;
	assert(ret);
	unused_block_count--;
	unused_block = unused_block->next_block;
	ret->prev_block = ret->next_block = NULL;
	if ( unused_block )
		unused_block->prev_block = NULL;
	ret->information |= BCACHE_USED;
	LinkBlock(ret);
	blocks_used++;
	return ret;
}

void BlockCache::ReleaseBlock(BlockCacheBlock* block)
{
	ScopedLock lock(&bcache_mutex);
	assert(block->information & BCACHE_PRESENT);
	assert(block->information & BCACHE_USED);
	blocks_used--;
	if ( blocks_per_area < unused_block_count )
	{
		blocks_allocated--;
		uint8_t* block_data = BlockDataUnlocked(block);
		addr_t block_data_addr = Memory::Unmap((addr_t) block_data);
		Page::Put(block_data_addr, PAGE_USAGE_FILESYSTEM_CACHE);
		// TODO: We leak this block's meta information here. Rather, we should
		// put this block into a list of non-present blocks so we can reuse it
		// later and reallocate a physical frame for it - then we will just
		// reuse the block's meta information.
		block->information &= ~(BCACHE_USED | BCACHE_PRESENT);
		return;
	}
	UnlinkBlock(block);
	unused_block_count++;
	if ( unused_block )
		unused_block->prev_block = block;
	block->next_block = unused_block;
	block->prev_block = NULL;
	block->information &= ~BCACHE_USED;
	unused_block = block;
}

uint8_t* BlockCache::BlockData(BlockCacheBlock* block)
{
	ScopedLock lock(&bcache_mutex);
	return BlockDataUnlocked(block);
}

uint8_t* BlockCache::BlockDataUnlocked(BlockCacheBlock* block)
{
	uintptr_t block_id = BlockIdOfBlockInformation(block->information);
	uintptr_t area_num = AreaOfBlockId(block_id, blocks_per_area);
	uintptr_t area_off = BlockOfBlockId(block_id, blocks_per_area);
	return areas[area_num].data + area_off * Page::Size();
}

void BlockCache::MarkUsed(BlockCacheBlock* block)
{
	ScopedLock lock(&bcache_mutex);
	UnlinkBlock(block);
	LinkBlock(block);
}

void BlockCache::MarkModified(BlockCacheBlock* block)
{
	ScopedLock lock(&bcache_mutex);
	UnlinkBlock(block);
	LinkBlock(block);
	block->information |= BCACHE_MODIFIED;
}

void BlockCache::UnlinkBlock(BlockCacheBlock* block)
{
	(block->prev_block ? block->prev_block->next_block : mru_block) = block->next_block;
	(block->next_block ? block->next_block->prev_block : lru_block) = block->prev_block;
	block->next_block = block->prev_block = NULL;
}

void BlockCache::LinkBlock(BlockCacheBlock* block)
{
	assert(!(block->next_block || block == lru_block));
	assert(!(block->prev_block || block == mru_block));
	block->prev_block = NULL;
	block->next_block = mru_block;
	if ( mru_block )
		mru_block->prev_block = block;
	mru_block = block;
	if ( !lru_block )
		lru_block = block;
}

bool BlockCache::AddArea()
{
	if ( areas_used == areas_length )
	{
		size_t new_length = areas_length ? 2 * areas_length : 8UL;
		size_t new_size = new_length * sizeof(BlockCacheArea);
		BlockCacheArea* new_areas = (BlockCacheArea*) realloc(areas, new_size);
		if ( !new_areas )
			return false;
		areas = new_areas;
		areas_length = new_length;
	}

	size_t area_memory_size = Page::Size() * blocks_per_area;
	int prot = PROT_KREAD | PROT_KWRITE;

	BlockCacheArea* area = &areas[areas_used];
	if ( !AllocateKernelAddress(&area->addralloc, area_memory_size) )
		goto cleanup_done;
	if ( !(area->blocks = new BlockCacheBlock[blocks_per_area]) )
		goto cleanup_addralloc;
	if ( !Memory::MapRange(area->addralloc.from, area->addralloc.size, prot, PAGE_USAGE_FILESYSTEM_CACHE) )
		goto cleanup_blocks;
	Memory::Flush();
	blocks_allocated += blocks_per_area;

	// Add all our new blocks into the unused block linked list.
	for ( size_t i = blocks_per_area; i != 0; i-- )
	{
		size_t index = i - 1;
		BlockCacheBlock* block = &area->blocks[index];
		uintptr_t blockid = MakeBlockId(areas_used, blocks_per_area, index);
		block->information = MakeBlockInformation(blockid, BCACHE_PRESENT);
		block->fcache = NULL;
		block->next_block = unused_block;
		block->prev_block = NULL;
		if ( unused_block )
			unused_block->prev_block = block;
		unused_block = block;
		unused_block_count++;
	}

	area->data = (uint8_t*) area->addralloc.from;
	for ( size_t i = 0; i < area_memory_size; i++ )
		area->data[i] = 0xCC;
	return areas_used++, true;

cleanup_blocks:
	delete[] area->blocks;
cleanup_addralloc:
	FreeKernelAddress(&area->addralloc);
cleanup_done:
	return false;
}

static BlockCache* kernel_block_cache = NULL;

/*static*/ void FileCache::Init()
{
	if ( !(kernel_block_cache = new BlockCache()) )
		Panic("Unable to allocate kernel block cache");
}

FileCache::FileCache(/*FileCacheBackend* backend*/)
{
	assert(kernel_block_cache);
	file_size = 0;
	file_written = 0;
	blocks = NULL;
	blocks_used = 0;
	blocks_length = 0;
	fcache_mutex = KTHREAD_MUTEX_INITIALIZER;
	modified = false;
	modified_size = false;
}

FileCache::~FileCache()
{
	for ( size_t i = 0; i < blocks_used; i++ )
		kernel_block_cache->ReleaseBlock(blocks[i]);
	free(blocks);
}

int FileCache::sync(ioctx_t* /*ctx*/)
{
	ScopedLock lock(&fcache_mutex);
	return Synchronize() ? 0 : -1;
}

bool FileCache::Synchronize()
{
	//if ( !backend )
	if ( true )
		return true;
	if ( !(modified || modified_size ) )
		return true;
	// TODO: Write out all dirty blocks and ask the next level to sync as well.
	return true;
}

void FileCache::InitializeFileData(off_t to_where)
{
	while ( file_written < to_where )
	{
		off_t left = to_where - file_written;
		size_t block_off = (size_t) (file_written % Page::Size());
		size_t block_num = (size_t) (file_written / Page::Size());
		size_t block_left = Page::Size() - block_off;
		size_t amount_to_set = (uintmax_t) left < (uintmax_t) block_left ?
		                       (size_t) left : block_left;
		assert(block_num < blocks_used);
		BlockCacheBlock* block = blocks[block_num];
		uint8_t* block_data = kernel_block_cache->BlockData(block);
		uint8_t* data = block_data + block_off;
		memset(data, 0, amount_to_set);
		file_written += (off_t) amount_to_set;
		kernel_block_cache->MarkModified(block);
	}
}

ssize_t FileCache::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	ScopedLock lock(&fcache_mutex);
	if ( off < 0 )
		return errno = EINVAL, -1;
	if ( file_size <= off )
		return 0;
	off_t available_bytes = file_size - off;
	if ( (uintmax_t) available_bytes < (uintmax_t) count )
		count = available_bytes;
	if ( (size_t) SSIZE_MAX < count )
		count = (size_t) SSIZE_MAX;
	size_t sofar = 0;
	while ( sofar < count )
	{
		off_t current_off = off + (off_t) sofar;
		size_t left = count - sofar;
		size_t block_off = (size_t) (current_off % Page::Size());
		size_t block_num = (size_t) (current_off / Page::Size());
		size_t block_left = Page::Size() - block_off;
		size_t amount_to_copy = left < block_left ? left : block_left;
		assert(block_num < blocks_used);
		BlockCacheBlock* block = blocks[block_num];
		const uint8_t* block_data = kernel_block_cache->BlockData(block);
		const uint8_t* src_data = block_data + block_off;
		uint8_t* dest_buf = buf + sofar;
		off_t end_at = current_off + (off_t) amount_to_copy;
		if ( file_written < end_at )
			InitializeFileData(end_at);
		if ( !ctx->copy_to_dest(dest_buf, src_data, amount_to_copy) )
			return sofar ? (ssize_t) sofar : -1;
		sofar += amount_to_copy;
		kernel_block_cache->MarkUsed(block);
	}
	return (ssize_t) sofar;
}

ssize_t FileCache::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off)
{
	ScopedLock lock(&fcache_mutex);
	if ( off < 0 )
		return errno = EINVAL, -1;
	off_t available_growth = OFF_MAX - off;
	if ( (uintmax_t) available_growth < (uintmax_t) count )
		count = (size_t) available_growth;
	// TODO: Rather than doing an EOF - shouldn't errno be set to something like
	//       "Hey, the filesize limit has been reached"?
	if ( (size_t) SSIZE_MAX < count )
		count = (size_t) SSIZE_MAX;
	off_t write_end = off + (off_t) count;
	if ( file_size < write_end && !ChangeSize(write_end, false) )
	{
		if ( file_size < off )
			return -1;
		count = (size_t) (file_size - off);
		write_end = off + (off_t) count;
	}
	assert(write_end <= file_size);
	size_t sofar = 0;
	while ( sofar < count )
	{
		off_t current_off = off + (off_t) sofar;
		size_t left = count - sofar;
		size_t block_off = (size_t) (current_off % Page::Size());
		size_t block_num = (size_t) (current_off / Page::Size());
		size_t block_left = Page::Size() - block_off;
		size_t amount_to_copy = left < block_left ? left : block_left;
		assert(block_num < blocks_used);
		BlockCacheBlock* block = blocks[block_num];
		uint8_t* block_data = kernel_block_cache->BlockData(block);
		uint8_t* data = block_data + block_off;
		const uint8_t* src_buf = buf + sofar;
		off_t begin_at = off + (off_t) sofar;
		off_t end_at = current_off + (off_t) amount_to_copy;
		if ( file_written < begin_at )
			InitializeFileData(begin_at);
		modified = true; /* Unconditionally - copy_from_src can fail midway. */
		if ( !ctx->copy_from_src(data, src_buf, amount_to_copy) )
			return sofar ? (ssize_t) sofar : -1;
		if ( file_written < end_at )
			file_written = end_at;
		sofar += amount_to_copy;
		kernel_block_cache->MarkModified(block);
	}
	return (ssize_t) sofar;
}

int FileCache::truncate(ioctx_t* /*ctx*/, off_t length)
{
	ScopedLock lock(&fcache_mutex);
	return ChangeSize(length, true) ? 0 : -1;
}

off_t FileCache::GetFileSize()
{
	ScopedLock lock(&fcache_mutex);
	return file_size;
}

off_t FileCache::lseek(ioctx_t* /*ctx*/, off_t offset, int whence)
{
	ScopedLock lock(&fcache_mutex);
	if ( whence == SEEK_SET )
		return offset;
	if ( whence == SEEK_END )
		return (off_t) file_size + offset;
	return errno = EINVAL, -1;
}

//bool FileCache::ChangeBackend(FileCacheBackend* backend, bool sync_old)
//{
//}

bool FileCache::ChangeSize(off_t new_size, bool exact)
{
	if ( file_size == new_size && !exact )
		return true;

	off_t numblocks_off_t = (new_size + Page::Size() - 1) / Page::Size();
	// TODO: An alternative datastructure (perhaps tree like) will decrease the
	// memory consumption of this pointer list as well as avoid this filesize
	// restriction on 32-bit platforms.
	if ( (uintmax_t) SIZE_MAX < (uintmax_t) numblocks_off_t )
		return errno = EFBIG, false;
	size_t numblocks = (size_t) numblocks_off_t;
	if ( !ChangeNumBlocks(numblocks, exact) )
		return false;
	if ( new_size < file_written )
		file_written = new_size;
	file_size = new_size;
	modified_size = true;
	return true;
}

bool FileCache::ChangeNumBlocks(size_t new_numblocks, bool exact)
{
	if ( new_numblocks == blocks_used && !exact )
		return true;

	// Release blocks if the file has decreased in size.
	if ( new_numblocks < blocks_used )
	{
		for ( size_t i = new_numblocks; i < blocks_used; i++ )
			kernel_block_cache->ReleaseBlock(blocks[i]);
		blocks_used = new_numblocks;
	}

	// If needed, adapt the length of the array containing block pointers.
	if ( new_numblocks < blocks_length )
		exact = true;
	size_t new_blocks_length = 2 * blocks_length;
	if ( exact || new_blocks_length < new_numblocks )
		new_blocks_length = new_numblocks;
	size_t size = sizeof(BlockCacheBlock*) * new_blocks_length;
	BlockCacheBlock** new_blocks = (BlockCacheBlock**) realloc(blocks, size);
	if ( !new_blocks )
	{
		if ( blocks_length < new_blocks_length )
			return false;
	}
	else
	{
		blocks = new_blocks;
		blocks_length = new_blocks_length;
	}

	assert(new_numblocks <= blocks_length);
	assert(!blocks_length || blocks);

	// Acquire more blocks if the file has increased in size.
	for ( size_t i = blocks_used; i < new_numblocks; i++ )
	{
		if ( !(blocks[i] = kernel_block_cache->AcquireBlock()) )
		{
			for ( size_t n = blocks_used; n < i; n++ )
				kernel_block_cache->ReleaseBlock(blocks[n]);
			return false;
		}
		blocks_used++;
	}

	return true;
}

} // namespace Sortix
