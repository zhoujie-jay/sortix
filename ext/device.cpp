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
 * device.cpp
 * Block device.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "block.h"
#include "device.h"
#include "ioleast.h"

void* Device__SyncThread(void* ctx)
{
	((Device*) ctx)->SyncThread();
	return NULL;
}

Device::Device(int fd, const char* path, uint32_t block_size, bool write)
{
	// sync_thread unset.
	this->sync_thread_cond = PTHREAD_COND_INITIALIZER;
	this->sync_thread_idle_cond = PTHREAD_COND_INITIALIZER;
	this->sync_thread_lock = PTHREAD_MUTEX_INITIALIZER;
	this->mru_block = NULL;
	this->lru_block = NULL;
	this->dirty_block = NULL;
	for ( size_t i = 0; i < DEVICE_HASH_LENGTH; i++ )
		hash_blocks[i] = NULL;
	struct stat st;
	fstat(fd, &st);
	this->device_size = st.st_size;
	this->path = path;
	this->block_size = block_size;
	this->fd = fd;
	this->write = write;
	this->has_sync_thread = false;
	this->sync_thread_should_exit = false;
	this->sync_in_transit = false;
	this->block_count = 0;
#ifdef __sortix__
	// TODO: This isn't scaleable if there's multiple filesystems mounted.
	size_t memory;
	memstat(NULL, &memory);
	this->block_limit = (memory / 10) / block_size;
#else
	this->block_limit = 32768;
#endif
}

Device::~Device()
{
	if ( has_sync_thread )
	{
		pthread_mutex_lock(&sync_thread_lock);
		sync_thread_should_exit = true;
		pthread_cond_signal(&sync_thread_cond);
		pthread_mutex_unlock(&sync_thread_lock);
		pthread_join(sync_thread, NULL);
		has_sync_thread = false;
	}
	Sync();
	while ( mru_block )
		delete mru_block;
	close(fd);
}

void Device::SpawnSyncThread()
{
	if ( this->has_sync_thread )
		return;
	this->has_sync_thread = write &&
		pthread_create(&this->sync_thread, NULL, Device__SyncThread, this) == 0;
}

Block* Device::AllocateBlock()
{
	if ( block_limit <= block_count )
	{
		for ( Block* block = lru_block; block; block = block->prev_block )
		{
			if ( block->reference_count )
				continue;
			block->Destruct(); // Syncs.
			return block;
		}
	}
	uint8_t* data = new uint8_t[block_size];
	if ( !data ) // TODO: Use operator new nothrow!
		return NULL;
	Block* block = new Block();
	if ( !block ) // TODO: Use operator new nothrow!
		return delete[] data, (Block*) NULL;
	block->block_data = data;
	block_count++;
	return block;
}

Block* Device::GetBlock(uint32_t block_id)
{
	if ( Block* block = GetCachedBlock(block_id) )
		return block;
	Block* block = AllocateBlock();
	if ( !block )
		return NULL;
	block->Construct(this, block_id);
	off_t file_offset = (off_t) block_size * (off_t) block_id;
	preadall(fd, block->block_data, block_size, file_offset);
	block->Prelink();
	return block;
}

Block* Device::GetBlockZeroed(uint32_t block_id)
{
	assert(write);
	if ( Block* block = GetCachedBlock(block_id) )
	{
		block->BeginWrite();
		memset(block->block_data, 0, block_size);
		block->FinishWrite();
		return block;
	}
	Block* block = AllocateBlock();
	if ( !block )
		return NULL;
	block->Construct(this, block_id);
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
	if ( has_sync_thread )
	{
		pthread_mutex_lock(&sync_thread_lock);
		while ( dirty_block || sync_in_transit )
			pthread_cond_wait(&sync_thread_cond, &sync_thread_lock);
		pthread_mutex_unlock(&sync_thread_lock);
		fsync(fd);
		return;
	}

	while ( dirty_block )
		dirty_block->Sync();
	fsync(fd);
}

void Device::SyncThread()
{
	uint8_t transit_block_data[block_size];
	pthread_mutex_lock(&sync_thread_lock);
	while ( true )
	{
		while ( !(dirty_block || sync_thread_should_exit) )
			pthread_cond_wait(&sync_thread_cond, &sync_thread_lock);
		if ( sync_thread_should_exit )
			break;

		Block* block = dirty_block;

		if ( block->next_dirty )
			block->next_dirty->prev_dirty = NULL;
		dirty_block = block->next_dirty;
		block->next_dirty = NULL;

		block->dirty = false;
		block->is_in_transit = true;
		sync_in_transit = true;

		pthread_mutex_unlock(&sync_thread_lock);

		pthread_mutex_lock(&block->modify_lock);
		memcpy(transit_block_data, block->block_data, block_size);
		pthread_mutex_unlock(&block->modify_lock);

		off_t offset = (off_t) block_size * (off_t) block->block_id;
		pwriteall(fd, transit_block_data, block_size, offset);

		pthread_mutex_lock(&sync_thread_lock);
		block->is_in_transit = false;
		sync_in_transit = false;
		pthread_cond_signal(&block->transit_done_cond);
		if ( !dirty_block )
			pthread_cond_signal(&sync_thread_idle_cond);
	}
	pthread_mutex_unlock(&sync_thread_lock);
}
