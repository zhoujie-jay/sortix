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
 * blockgroup.cpp
 * Filesystem block group.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "block.h"
#include "blockgroup.h"
#include "device.h"
#include "filesystem.h"
#include "util.h"

BlockGroup::BlockGroup(Filesystem* filesystem, uint32_t group_id)
{
	this->data_block = NULL;
	this->data = NULL;
	this->filesystem = filesystem;
	this->block_bitmap_chunk = NULL;
	this->inode_bitmap_chunk = NULL;
	this->reference_count = 1;
	this->group_id = group_id;
	this->block_alloc_chunk = 0;
	this->inode_alloc_chunk = 0;
	this->block_bitmap_chunk_i = 0;
	this->inode_bitmap_chunk_i = 0;
	this->first_block_id = filesystem->sb->s_first_data_block +
	                       filesystem->sb->s_blocks_per_group * group_id;
	this->first_inode_id = 1 +
	                       filesystem->sb->s_inodes_per_group * group_id;
	this->num_blocks = group_id+1== filesystem->num_groups ?
	                   filesystem->num_blocks - first_block_id :
	                   filesystem->sb->s_blocks_per_group;
	this->num_inodes = group_id+1== filesystem->num_groups ?
	                   filesystem->num_inodes - first_inode_id :
	                   filesystem->sb->s_inodes_per_group;
	size_t num_chunk_bits = filesystem->block_size * 8UL;
	this->num_block_bitmap_chunks = divup(num_blocks, (uint32_t) num_chunk_bits);
	this->num_inode_bitmap_chunks = divup(num_inodes, (uint32_t) num_chunk_bits);
	this->dirty = false;
}

BlockGroup::~BlockGroup()
{
	Sync();
	if ( block_bitmap_chunk )
		block_bitmap_chunk->Unref();
	if ( inode_bitmap_chunk )
		inode_bitmap_chunk->Unref();
	if ( data_block )
		data_block->Unref();
	filesystem->block_groups[group_id] = NULL;
}

uint32_t BlockGroup::AllocateBlock()
{
	if ( !filesystem->device->write )
		return errno = EROFS, 0;
	if ( !data->bg_free_blocks_count )
		return errno = ENOSPC, 0;
	size_t num_chunk_bits = filesystem->block_size * 8UL;
	uint32_t begun_chunk = block_alloc_chunk;
	for ( uint32_t i = 0; i < num_block_bitmap_chunks; i++ )
	{
		block_alloc_chunk = (begun_chunk + i) % num_block_bitmap_chunks;
		bool last = block_alloc_chunk + 1 == num_block_bitmap_chunks;
		if ( !block_bitmap_chunk )
		{
			uint32_t block_id = data->bg_block_bitmap + block_alloc_chunk;
			block_bitmap_chunk = filesystem->device->GetBlock(block_id);
			if ( !block_bitmap_chunk )
				return 0;
			block_bitmap_chunk_i = 0;
		}
		uint32_t chunk_offset = block_alloc_chunk * num_chunk_bits;
		uint8_t* chunk_bits = block_bitmap_chunk->block_data;
		size_t num_bits = last ? num_blocks - chunk_offset : num_chunk_bits;
		// TODO: This can be made faster by caching if previous bits were set.
		for ( ; block_bitmap_chunk_i < num_bits; block_bitmap_chunk_i++ )
		{
			if ( !checkbit(chunk_bits, block_bitmap_chunk_i) )
			{
				block_bitmap_chunk->BeginWrite();
				setbit(chunk_bits, block_bitmap_chunk_i);
				block_bitmap_chunk->FinishWrite();
				BeginWrite();
				data->bg_free_blocks_count--;
				FinishWrite();
				filesystem->BeginWrite();
				filesystem->sb->s_free_blocks_count--;
				filesystem->FinishWrite();
				uint32_t group_block_id = chunk_offset + block_bitmap_chunk_i++;
				uint32_t block_id = first_block_id + group_block_id;
				return block_id;
			}
		}
		block_bitmap_chunk->Unref();
		block_bitmap_chunk = NULL;
	}
	BeginWrite();
	data->bg_free_blocks_count = 0;
	FinishWrite();
	return errno = ENOSPC, 0;
}

uint32_t BlockGroup::AllocateInode()
{
	if ( !filesystem->device->write )
		return errno = EROFS, 0;
	if ( !data->bg_free_inodes_count )
		return errno = ENOSPC, 0;
	size_t num_chunk_bits = filesystem->block_size * 8UL;
	uint32_t begun_chunk = inode_alloc_chunk;
	for ( uint32_t i = 0; i < num_inode_bitmap_chunks; i++ )
	{
		inode_alloc_chunk = (begun_chunk + i) % num_inode_bitmap_chunks;
		bool last = inode_alloc_chunk + 1 == num_inode_bitmap_chunks;
		if ( !inode_bitmap_chunk )
		{
			uint32_t block_id = data->bg_inode_bitmap + inode_alloc_chunk;
			inode_bitmap_chunk = filesystem->device->GetBlock(block_id);
			if ( !inode_bitmap_chunk )
				return 0;
			inode_bitmap_chunk_i = 0;
		}
		uint32_t chunk_offset = inode_alloc_chunk * num_chunk_bits;
		uint8_t* chunk_bits = inode_bitmap_chunk->block_data;
		size_t num_bits = last ? num_inodes - chunk_offset : num_chunk_bits;
		// TODO: This can be made faster by caching if previous bits were set.
		for ( ; inode_bitmap_chunk_i < num_bits; inode_bitmap_chunk_i++ )
		{
			if ( !checkbit(chunk_bits, inode_bitmap_chunk_i) )
			{
				inode_bitmap_chunk->BeginWrite();
				setbit(chunk_bits, inode_bitmap_chunk_i);
				inode_bitmap_chunk->FinishWrite();
				BeginWrite();
				data->bg_free_inodes_count--;
				FinishWrite();
				filesystem->BeginWrite();
				filesystem->sb->s_free_inodes_count--;
				filesystem->FinishWrite();
				uint32_t group_inode_id = chunk_offset + inode_bitmap_chunk_i++;
				uint32_t inode_id = first_inode_id + group_inode_id;
				return inode_id;
			}
		}
		inode_bitmap_chunk->Unref();
		inode_bitmap_chunk = NULL;
	}
	BeginWrite();
	data->bg_free_inodes_count = 0;
	FinishWrite();
	return errno = ENOSPC, 0;
}

void BlockGroup::FreeBlock(uint32_t block_id)
{
	assert(filesystem->device->write);
	block_id -= first_block_id;
	size_t num_chunk_bits = filesystem->block_size * 8UL;
	uint32_t chunk_id = block_id / num_chunk_bits;
	uint32_t chunk_bit = block_id % num_chunk_bits;
	if ( !block_bitmap_chunk || chunk_id != block_alloc_chunk )
	{
		if ( block_bitmap_chunk )
			block_bitmap_chunk->Unref();
		block_alloc_chunk = chunk_id;
		uint32_t block_id = data->bg_block_bitmap + block_alloc_chunk;
		block_bitmap_chunk = filesystem->device->GetBlock(block_id);
		block_bitmap_chunk_i = 0;
	}

	block_bitmap_chunk->BeginWrite();
	uint8_t* chunk_bits = block_bitmap_chunk->block_data;
	clearbit(chunk_bits, chunk_bit);
	block_bitmap_chunk->FinishWrite();
	if ( chunk_bit < inode_bitmap_chunk_i )
		block_bitmap_chunk_i = chunk_bit;
	BeginWrite();
	data->bg_free_blocks_count++;
	FinishWrite();
	filesystem->BeginWrite();
	filesystem->sb->s_free_blocks_count++;
	filesystem->FinishWrite();
}

void BlockGroup::FreeInode(uint32_t inode_id)
{
	assert(filesystem->device->write);
	inode_id -= first_inode_id;
	size_t num_chunk_bits = filesystem->block_size * 8UL;
	uint32_t chunk_id = inode_id / num_chunk_bits;
	uint32_t chunk_bit = inode_id % num_chunk_bits;
	if ( !inode_bitmap_chunk || chunk_id != inode_alloc_chunk )
	{
		if ( inode_bitmap_chunk )
			inode_bitmap_chunk->Unref();
		inode_alloc_chunk = chunk_id;
		uint32_t block_id = data->bg_inode_bitmap + inode_alloc_chunk;
		inode_bitmap_chunk = filesystem->device->GetBlock(block_id);
		inode_bitmap_chunk_i = 0;
	}

	inode_bitmap_chunk->BeginWrite();
	uint8_t* chunk_bits = inode_bitmap_chunk->block_data;
	clearbit(chunk_bits, chunk_bit);
	inode_bitmap_chunk->FinishWrite();
	if ( chunk_bit < inode_bitmap_chunk_i )
		inode_bitmap_chunk_i = chunk_bit;
	BeginWrite();
	data->bg_free_inodes_count++;
	FinishWrite();
	filesystem->BeginWrite();
	filesystem->sb->s_free_inodes_count++;
	filesystem->FinishWrite();
}

void BlockGroup::Refer()
{
	// TODO
}

void BlockGroup::Unref()
{
	// TODO
}

void BlockGroup::Sync()
{
	if ( block_bitmap_chunk )
		block_bitmap_chunk->Sync();
	if ( inode_bitmap_chunk )
		inode_bitmap_chunk->Sync();
	if ( dirty )
		data_block->Sync();
	dirty = false;
}

void BlockGroup::BeginWrite()
{
	data_block->BeginWrite();
}

void BlockGroup::FinishWrite()
{
	dirty = true;
	data_block->FinishWrite();
	Use();
}

void BlockGroup::Use()
{
}
