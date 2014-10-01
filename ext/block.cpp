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

    block.cpp
    Blocks in the filesystem.

*******************************************************************************/

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include "block.h"
#include "device.h"
#include "ioleast.h"

Block::Block(Device* device, uint32_t block_id)
{
	this->prev_block = NULL;
	this->next_block = NULL;
	this->prev_hashed = NULL;
	this->next_hashed = NULL;
	this->prev_dirty = NULL;
	this->next_dirty = NULL;
	this->device = device;
	this->reference_count = 1;
	this->block_id = block_id;
	this->dirty = false;
	this->block_data = 0;
}

Block::~Block()
{
	Sync();
	Unlink();
	delete[] block_data;
}

void Block::Refer()
{
	reference_count++;
}

void Block::Unref()
{
	if ( !--reference_count )
#if 0
		delete this;
#else
		{};
#endif
}

void Block::Sync()
{
	if ( !dirty )
		return;
	dirty = false;
	(prev_dirty ? prev_dirty->next_dirty : device->dirty_block) = next_dirty;
	if ( next_dirty )
		next_dirty->prev_dirty = prev_dirty;
	prev_dirty = NULL;
	next_dirty = NULL;
	if ( !device->write )
		return;
	off_t file_offset = (off_t) device->block_size * (off_t) block_id;
	pwriteall(device->fd, block_data, device->block_size, file_offset);
}

void Block::Dirty()
{
	if ( !dirty )
	{
		dirty = true;
		prev_dirty = NULL;
		next_dirty = device->dirty_block;
		if ( next_dirty )
			next_dirty->prev_dirty = this;
		device->dirty_block = this;
	}
	Use();
}

void Block::Use()
{
	Unlink();
	Prelink();
}

void Block::Unlink()
{
	(prev_block ? prev_block->next_block : device->mru_block) = next_block;
	(next_block ? next_block->prev_block : device->lru_block) = prev_block;
	size_t bin = block_id % DEVICE_HASH_LENGTH;
	(prev_hashed ? prev_hashed->next_hashed : device->hash_blocks[bin]) = next_hashed;
	if ( next_hashed ) next_hashed->prev_hashed = prev_hashed;
}

void Block::Prelink()
{
	prev_block = NULL;
	next_block = device->mru_block;
	if ( device->mru_block )
		device->mru_block->prev_block = this;
	device->mru_block = this;
	if ( !device->lru_block )
		device->lru_block = this;
	size_t bin = block_id % DEVICE_HASH_LENGTH;
	prev_hashed = NULL;
	next_hashed = device->hash_blocks[bin];
	device->hash_blocks[bin] = this;
	if ( next_hashed )
		next_hashed->prev_hashed = this;
}
