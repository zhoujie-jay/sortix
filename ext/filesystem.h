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
 * filesystem.h
 * Filesystem.
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

class BlockGroup;
class Device;
class Inode;

static const size_t INODE_HASH_LENGTH = 1 << 16;

class Filesystem
{
public:
	Filesystem(Device* device, const char* mount_path);
	~Filesystem();

public:
	Block* sb_block;
	struct ext_superblock* sb;
	Device* device;
	BlockGroup** block_groups;
	const char* mount_path;
	uint32_t block_size;
	uint32_t inode_size;
	uint32_t num_blocks;
	uint32_t num_groups;
	uint32_t num_inodes;
	Inode* mru_inode;
	Inode* lru_inode;
	Inode* dirty_inode;
	Inode* hash_inodes[INODE_HASH_LENGTH];
	time_t mtime_realtime;
	time_t mtime_monotonic;
	bool dirty;

public:
	BlockGroup* GetBlockGroup(uint32_t group_id);
	Inode* GetInode(uint32_t inode_id);
	uint32_t AllocateBlock(BlockGroup* preferred = NULL);
	uint32_t AllocateInode(BlockGroup* preferred = NULL);
	void FreeBlock(uint32_t block_id);
	void FreeInode(uint32_t inode_id);
	void BeginWrite();
	void FinishWrite();
	void Sync();

};

#endif
