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

    block.h
    Blocks in the filesystem.

*******************************************************************************/

#ifndef BLOCK_H
#define BLOCK_H

class Device;

class Block
{
public:
	Block(Device* device, uint32_t block_id);
	~Block();

public:
	Block* prev_block;
	Block* next_block;
	Block* prev_hashed;
	Block* next_hashed;
	Block* prev_dirty;
	Block* next_dirty;
	Device* device;
	size_t reference_count;
	uint32_t block_id;
	bool dirty;
	uint8_t* block_data;

public:
	void Refer();
	void Unref();
	void Sync();
	void Dirty();
	void Use();
	void Unlink();
	void Prelink();

};

#endif
