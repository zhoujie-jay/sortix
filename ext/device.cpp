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

    device.cpp
    Block device.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "block.h"
#include "device.h"
#include "ioleast.h"

Device::Device(int fd, uint32_t block_size, bool write)
{
	this->write = write;
	this->fd = fd;
	this->block_size = block_size;
	struct stat st;
	fstat(fd, &st);
	this->device_size = st.st_size;
	this->mru_block = NULL;
	this->lru_block = NULL;
	this->dirty_block = NULL;
	for ( size_t i = 0; i < DEVICE_HASH_LENGTH; i++ )
		hash_blocks[i] = NULL;
}

Device::~Device()
{
	Sync();
	while ( mru_block )
		delete mru_block;
}

Block* Device::GetBlock(uint32_t block_id)
{
	if ( Block* block = GetCachedBlock(block_id) )
		return block;
	Block* block = new Block(this, block_id);
	block->block_data = new uint8_t[block_size];
	off_t file_offset = (off_t) block_size * (off_t) block_id;
	preadall(fd, block->block_data, block_size, file_offset);
	block->Prelink();
	return block;
}

Block* Device::GetBlockZeroed(uint32_t block_id)
{
	if ( Block* block = GetCachedBlock(block_id) )
	{
		block->BeginWrite();
		memset(block->block_data, 0, block_size);
		block->FinishWrite();
		return block;
	}
	Block* block = new Block(this, block_id);
	block->block_data = new uint8_t[block_size];
	memset(block->block_data, 0, block_size);
	block->Prelink();
	block->BeginWrite();
	block->FinishWrite();
	return block;
}

Block* Device::GetCachedBlock(uint32_t block_id)
{
	size_t bin = block_id % DEVICE_HASH_LENGTH;
	for ( Block* iter = hash_blocks[bin]; iter; iter = iter->next_hashed )
		if ( iter->block_id == block_id )
			return iter->Refer(), iter;
	return NULL;
}

void Device::Sync()
{
	while ( dirty_block )
		dirty_block->Sync();
}
