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
 * block.cpp
 * Blocks in the filesystem.
 */

#include <sys/types.h>

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "block.h"
#include "device.h"
#include "ioleast.h"

Block::Block()
{
	this->block_data = NULL;
}

Block::Block(Device* device, uint32_t block_id)
{
	Construct(device, block_id);
}

void Block::Construct(Device* device, uint32_t block_id)
{
	this->modify_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	this->transit_done_cond = PTHREAD_COND_INITIALIZER;
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
	this->is_in_transit = false;
}

Block::~Block()
{
	Destruct();
	delete[] block_data;
}

void Block::Destruct()
{
	Sync();
	Unlink();
}

void Block::Refer()
{
	reference_count++;
}

void Block::Unref()
{
	if ( !--reference_count )
	{
#if 0
		device->block_count--;
		delete this;
#endif
	}
}

void Block::Sync()
{
	if ( device->has_sync_thread )
	{
		pthread_mutex_lock(&device->sync_thread_lock);
		while ( dirty || is_in_transit )
			pthread_cond_wait(&transit_done_cond, &device->sync_thread_lock);
		pthread_mutex_unlock(&device->sync_thread_lock);
		return;
	}

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

void Block::BeginWrite()
{
	assert(device->write);
	pthread_mutex_lock(&modify_lock);
}

void Block::FinishWrite()
{
	pthread_mutex_unlock(&modify_lock);
	pthread_mutex_lock(&device->sync_thread_lock);
	if ( !dirty )
	{
		dirty = true;
		prev_dirty = NULL;
		next_dirty = device->dirty_block;
		if ( next_dirty )
			next_dirty->prev_dirty = this;
		device->dirty_block = this;
		pthread_cond_signal(&device->sync_thread_cond);
	}
	pthread_mutex_unlock(&device->sync_thread_lock);
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
