/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    device.h
    Block device.

*******************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

class Block;

const size_t DEVICE_HASH_LENGTH = 1 << 16;

class Device
{
public:
	Device(int fd, uint32_t block_size, bool write);
	~Device();

public:
	Block* mru_block;
	Block* lru_block;
	Block* dirty_block;
	Block* hash_blocks[DEVICE_HASH_LENGTH];
	off_t device_size;
	uint32_t block_size;
	int fd;
	bool write;

public:
	Block* GetBlock(uint32_t block_id);
	Block* GetBlockZeroed(uint32_t block_id);
	Block* GetCachedBlock(uint32_t block_id);
	void Sync();

};

#endif
