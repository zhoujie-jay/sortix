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
 * block.h
 * Blocks in the filesystem.
 */

#ifndef BLOCK_H
#define BLOCK_H

class Device;

class Block
{
public:
	Block();
	Block(Device* device, uint32_t block_id);
	~Block();
	void Construct(Device* device, uint32_t block_id);
	void Destruct();

public:
	pthread_mutex_t modify_lock;
	pthread_cond_t transit_done_cond;
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
	bool is_in_transit;
	uint8_t* block_data;

public:
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
