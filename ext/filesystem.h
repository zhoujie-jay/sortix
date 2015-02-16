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

    filesystem.h
    Filesystem.

*******************************************************************************/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

class BlockGroup;
class Device;
class Inode;

const size_t INODE_HASH_LENGTH = 1 << 16;

class Filesystem
{
public:
	Filesystem(Device* device, const char* mount_path);
	~Filesystem();

public:
	struct ext_superblock* sb;
	Block* sb_block;
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
