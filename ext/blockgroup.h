/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

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

    blockgroup.h
    Filesystem block group.

*******************************************************************************/

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
	uint32_t num_block_bitmap_chunks;
	uint32_t num_inode_bitmap_chunks;
	uint32_t block_bitmap_chunk_i;
	uint32_t inode_bitmap_chunk_i;
	uint32_t num_blocks;
	uint32_t num_inodes;
	uint32_t first_block_id;
	uint32_t first_inode_id;
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
