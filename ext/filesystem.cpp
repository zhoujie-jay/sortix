/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    filesystem.cpp
    Filesystem.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "block.h"
#include "blockgroup.h"
#include "device.h"
#include "filesystem.h"
#include "inode.h"
#include "util.h"

Filesystem::Filesystem(Device* device)
{
	uint64_t sb_offset = 1024;
	uint32_t sb_block_id = sb_offset / device->block_size;
	sb_block = device->GetBlock(sb_block_id);
	sb = (struct ext_superblock*)
		(sb_block->block_data + sb_offset % device->block_size);
	this->device = device;
	block_groups = NULL;
	block_size = device->block_size;
	mru_inode = NULL;
	lru_inode = NULL;
	inode_size = this->sb->s_inode_size;
	num_blocks = sb->s_blocks_count;
	num_groups = divup(this->sb->s_blocks_count, this->sb->s_blocks_per_group);
	num_inodes = this->sb->s_inodes_count;
	dirty = false;

	struct timespec now_realtime, now_monotonic;
	clock_gettime(CLOCK_REALTIME, &now_realtime);
	clock_gettime(CLOCK_MONOTONIC, &now_monotonic);
	mtime_realtime = now_realtime.tv_sec;
	mtime_monotonic = now_monotonic.tv_sec;
	sb->s_mtime = mtime_realtime;
	sb->s_mnt_count++;
	sb->s_state = EXT2_ERROR_FS;
	Dirty();
	Sync();
}

Filesystem::~Filesystem()
{
	Sync();
	while ( mru_inode )
		delete mru_inode;
	for ( size_t i = 0; i < num_groups; i++ )
		delete block_groups[i];
	delete[] block_groups;
	sb->s_state = EXT2_VALID_FS;
	Dirty();
	Sync();
	sb_block->Unref();
}

void Filesystem::Dirty()
{
	dirty = true;
	sb_block->Dirty();
}

void Filesystem::Sync()
{
	for ( Inode* iter = mru_inode; iter; iter = iter->next_inode )
		iter->Sync();
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
	BlockGroup* group = new BlockGroup(this, group_id);

	size_t group_size = sizeof(ext_blockgrpdesc);
	uint32_t first_block_id = sb->s_first_data_block + 1 /* superblock */;
	uint32_t block_id = first_block_id + (group_id * group_size) / block_size;
	uint32_t offset = (group_id * group_size) % block_size;

	Block* block = device->GetBlock(block_id);
	group->data_block = block;
	uint8_t* buf = group->data_block->block_data + offset;
	group->data = (struct ext_blockgrpdesc*) buf;
	return block_groups[group_id] = group;
}

Inode* Filesystem::GetInode(uint32_t inode_id)
{
	assert(inode_id);
	assert(inode_id < num_inodes);

	for ( Inode* iter = mru_inode; iter; iter = iter->next_inode )
		if ( iter->inode_id == inode_id )
			return iter->Refer(), iter;

	Inode* inode = new Inode(this, inode_id);

	uint32_t group_id = (inode_id-1) / sb->s_inodes_per_group;
	uint32_t tabel_index = (inode_id-1) % sb->s_inodes_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	uint32_t tabel_block = group->data->bg_inode_table;
	group->Unref();
	uint32_t block_id = tabel_block + (tabel_index * inode_size) / block_size;
	uint32_t offset = (tabel_index * inode_size) % block_size;

	Block* block = device->GetBlock(block_id);
	inode->data_block = block;
	uint8_t* buf = inode->data_block->block_data + offset;
	inode->data = (struct ext_inode*) buf;
	inode->Prelink();

	return inode;
}

uint32_t Filesystem::AllocateBlock(BlockGroup* preferred)
{
	if ( !sb->s_free_blocks_count )
		return errno = ENOSPC, 0;
	if ( preferred )
		if ( uint32_t block_id = preferred->AllocateBlock() )
			return block_id;
	for ( uint32_t group_id = 0; group_id < num_groups; group_id++ )
		if ( uint32_t block_id = GetBlockGroup(group_id)->AllocateBlock() )
			return block_id;
	sb->s_free_blocks_count--;
	Dirty();
	return errno = ENOSPC, 0;
}

uint32_t Filesystem::AllocateInode(BlockGroup* preferred)
{
	if ( !sb->s_free_inodes_count )
		return errno = ENOSPC, 0;
	if ( preferred )
		if ( uint32_t inode_id = preferred->AllocateInode() )
			return inode_id;
	for ( uint32_t group_id = 0; group_id < num_groups; group_id++ )
		if ( uint32_t inode_id = GetBlockGroup(group_id)->AllocateInode() )
			return inode_id;
	sb->s_free_inodes_count--;
	Dirty();
	return errno = ENOSPC, 0;
}

void Filesystem::FreeBlock(uint32_t block_id)
{
	assert(block_id < num_blocks);
	uint32_t group_id = (block_id - sb->s_first_data_block) / sb->s_blocks_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	group->FreeBlock(block_id);
	group->Unref();
}

void Filesystem::FreeInode(uint32_t inode_id)
{
	assert(inode_id < num_inodes);
	uint32_t group_id = (inode_id-1) / sb->s_inodes_per_group;
	assert(group_id < num_groups);
	BlockGroup* group = GetBlockGroup(group_id);
	group->FreeInode(inode_id);
	group->Unref();
}
