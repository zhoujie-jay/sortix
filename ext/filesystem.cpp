/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * filesystem.cpp
 * Filesystem.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "block.h"
#include "blockgroup.h"
#include "device.h"
#include "filesystem.h"
#include "inode.h"
#include "util.h"

Filesystem::Filesystem(Device* device, const char* mount_path)
{
	uint64_t sb_offset = 1024;
	uint32_t sb_block_id = sb_offset / device->block_size;
	this->sb_block = device->GetBlock(sb_block_id);
	assert(sb_block); // TODO: This can fail.
	this->sb = (struct ext_superblock*)
		       (sb_block->block_data + sb_offset % device->block_size);
	this->device = device;
	this->block_groups = NULL;
	this->mount_path = mount_path;
	this->block_size = device->block_size;
	this->inode_size = this->sb->s_inode_size;
	this->num_blocks = this->sb->s_blocks_count;
	this->num_groups = divup(this->sb->s_blocks_count, this->sb->s_blocks_per_group);
	this->num_inodes = this->sb->s_inodes_count;
	this->mru_inode = NULL;
	this->lru_inode = NULL;
	this->dirty_inode = NULL;
	for ( size_t i = 0; i < INODE_HASH_LENGTH; i++ )
		this->hash_inodes[i] = NULL;
	struct timespec now_realtime, now_monotonic;
	clock_gettime(CLOCK_REALTIME, &now_realtime);
	clock_gettime(CLOCK_MONOTONIC, &now_monotonic);
	this->mtime_realtime = now_realtime.tv_sec;
	this->mtime_monotonic = now_monotonic.tv_sec;
	this->dirty = false;

	if ( device->write )
	{
		BeginWrite();
		sb->s_mtime = mtime_realtime;
		sb->s_mnt_count++;
		sb->s_state = EXT2_ERROR_FS;
		// TODO: Remove this temporarily compatibility when this driver is moved
		//       into the kernel and the the FUSE frontend is removed.
#ifdef __GLIBC__
		strncpy(sb->s_last_mounted, mount_path, sizeof(sb->s_last_mounted));
#else
		strlcpy(sb->s_last_mounted, mount_path, sizeof(sb->s_last_mounted));
#endif
		FinishWrite();
		Sync();
	}
}

Filesystem::~Filesystem()
{
	Sync();
	while ( mru_inode )
		delete mru_inode;
	for ( size_t i = 0; i < num_groups; i++ )
		delete block_groups[i];
	delete[] block_groups;
	if ( device->write )
	{
		BeginWrite();
		sb->s_state = EXT2_VALID_FS;
		FinishWrite();
		Sync();
	}
	sb_block->Unref();
}

void Filesystem::BeginWrite()
{
	sb_block->BeginWrite();
}

void Filesystem::FinishWrite()
{
	dirty = true;
	sb_block->FinishWrite();
}

void Filesystem::Sync()
{
	while ( dirty_inode )
		dirty_inode->Sync();
	// TODO: This can be made faster by maintaining a linked list of dirty block
	//       groups.
	for ( size_t i = 0; i < num_groups; i++ )
		if ( block_groups && block_groups[i] )
			block_groups[i]->Sync();
	if ( dirty )
	{
		// The correct real-time might not have been known when the filesystem
		// was mounted (perhaps during early system boot), so find out what time
		// it is now, how long ago we were mounted, and subtract to get the
		// correct mount time.
		struct timespec now_realtime, now_monotonic;
		clock_gettime(CLOCK_REALTIME, &now_realtime);
		clock_gettime(CLOCK_MONOTONIC, &now_monotonic);
		time_t since_boot = now_monotonic.tv_sec - mtime_monotonic;
		mtime_realtime = now_realtime.tv_sec - since_boot;

		sb->s_wtime = now_realtime.tv_sec;
		sb->s_mtime = mtime_realtime;
		sb_block->Sync();
		dirty = false;
	}
	device->Sync();
}

BlockGroup* Filesystem::GetBlockGroup(uint32_t group_id)
{
	assert(group_id < num_groups);
	if ( block_groups[group_id] )
		return block_groups[group_id]->Refer(), block_groups[group_id];

	size_t group_size = sizeof(ext_blockgrpdesc);
	uint32_t first_block_id = sb->s_first_data_block + 1 /* superblock */;
	uint32_t block_id = first_block_id + (group_id * group_size) / block_size;
	uint32_t offset = (group_id * group_size) % block_size;

	Block* block = device->GetBlock(block_id);
	if ( !block )
		return (BlockGroup*) NULL;
	BlockGroup* group = new BlockGroup(this, group_id);
	if ( !group ) // TODO: Use operator new nothrow!
		return block->Unref(), (BlockGroup*) NULL;
	group->data_block = block;
	uint8_t* buf = group->data_block->block_data + offset;
	group->data = (struct ext_blockgrpdesc*) buf;
	return block_groups[group_id] = group;
}

Inode* Filesystem::GetInode(uint32_t inode_id)
{
	if ( !inode_id || num_inodes <= inode_id )
		return errno = EBADF, (Inode*) NULL;
	if ( !inode_id )
		return errno = EBADF, (Inode*) NULL;

	size_t bin = inode_id % INODE_HASH_LENGTH;
	for ( Inode* iter = hash_inodes[bin]; iter; iter = iter->next_hashed )
		if ( iter->inode_id == inode_id )
			return iter->Refer(), iter;

	uint32_t group_id = (inode_id-1) / sb->s_inodes_per_group;
	uint32_t tabel_index = (inode_id-1) % sb->s_inodes_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	if ( !group )
		return (Inode*) NULL;
	uint32_t tabel_block = group->data->bg_inode_table;
	group->Unref();
	uint32_t block_id = tabel_block + (tabel_index * inode_size) / block_size;
	uint32_t offset = (tabel_index * inode_size) % block_size;

	Block* block = device->GetBlock(block_id);
	if ( !block )
		return (Inode*) NULL;
	Inode* inode = new Inode(this, inode_id);
	if ( !inode )
		return block->Unref(), (Inode*) NULL;
	inode->data_block = block;
	uint8_t* buf = inode->data_block->block_data + offset;
	inode->data = (struct ext_inode*) buf;
	inode->Prelink();

	return inode;
}

uint32_t Filesystem::AllocateBlock(BlockGroup* preferred)
{
	if ( !device->write )
		return errno = EROFS, 0;
	if ( !sb->s_free_blocks_count )
		return errno = ENOSPC, 0;
	if ( preferred )
		if ( uint32_t block_id = preferred->AllocateBlock() )
			return block_id;
	// TODO: This can be made faster by maintaining a linked list of block
	//       groups that definitely have free blocks.
	for ( uint32_t group_id = 0; group_id < num_groups; group_id++ )
		if ( uint32_t block_id = GetBlockGroup(group_id)->AllocateBlock() )
			return block_id;
	// TODO: This case should only be fit in the event of corruption. We should
	//       rebuild all these values upon filesystem mount instead so we know
	//       this can't happen. That also allows us to make the linked list
	//       requested above.
	BeginWrite();
	sb->s_free_blocks_count = 0;
	FinishWrite();
	return errno = ENOSPC, 0;
}

uint32_t Filesystem::AllocateInode(BlockGroup* preferred)
{
	if ( !device->write )
		return errno = EROFS, 0;
	if ( !sb->s_free_inodes_count )
		return errno = ENOSPC, 0;
	if ( preferred )
		if ( uint32_t inode_id = preferred->AllocateInode() )
			return inode_id;
	// TODO: This can be made faster by maintaining a linked list of block
	//       groups that definitely have free inodes.
	for ( uint32_t group_id = 0; group_id < num_groups; group_id++ )
		if ( uint32_t inode_id = GetBlockGroup(group_id)->AllocateInode() )
			return inode_id;
	// TODO: This case should only be fit in the event of corruption. We should
	//       rebuild all these values upon filesystem mount instead so we know
	//       this can't happen. That also allows us to make the linked list
	//       requested above.
	BeginWrite();
	sb->s_free_inodes_count = 0;
	FinishWrite();
	return errno = ENOSPC, 0;
}

void Filesystem::FreeBlock(uint32_t block_id)
{
	assert(device->write);
	assert(block_id);
	assert(block_id < num_blocks);
	uint32_t group_id = (block_id - sb->s_first_data_block) / sb->s_blocks_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	if ( !group )
		return;
	group->FreeBlock(block_id);
	group->Unref();
}

void Filesystem::FreeInode(uint32_t inode_id)
{
	assert(device->write);
	assert(inode_id);
	assert(inode_id < num_inodes);
	uint32_t group_id = (inode_id-1) / sb->s_inodes_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	if ( !group )
		return;
	group->FreeInode(inode_id);
	group->Unref();
}
