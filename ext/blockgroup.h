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
 * blockgroup.h
 * Filesystem block group.
 */

#ifndef BLOCKGROUP_H
#define BLOCKGROUP_H

class Block;
class Filesystem;

class BlockGroup
{
public:
	BlockGroup(Filesystem* filesystem, uint32_t group_id);
	~BlockGroup();

public:
	Block* data_block;
	struct ext_blockgrpdesc* data;
	Filesystem* filesystem;
	Block* block_bitmap_chunk;
	Block* inode_bitmap_chunk;
	size_t reference_count;
	uint32_t group_id;
	uint32_t block_alloc_chunk;
	uint32_t inode_alloc_chunk;
	uint32_t block_bitmap_chunk_i;
	uint32_t inode_bitmap_chunk_i;
	uint32_t first_block_id;
	uint32_t first_inode_id;
	uint32_t num_blocks;
	uint32_t num_inodes;
	uint32_t num_block_bitmap_chunks;
	uint32_t num_inode_bitmap_chunks;
	bool dirty;

public:
	uint32_t AllocateBlock();
	uint32_t AllocateInode();
	void FreeBlock(uint32_t block_id);
	void FreeInode(uint32_t inode_id);
	void Refer();
	void Unref();
	void Sync();
	void BeginWrite();
	void FinishWrite();
	void Use();
	void Unlink();
	void Prelink();

};

#endif
