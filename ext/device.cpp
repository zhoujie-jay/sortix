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
	this->write = write;
	this->fd = fd;
	this->path = path;
	this->block_size = block_size;
	struct stat st;
	fstat(fd, &st);
	this->device_size = st.st_size;
	this->mru_block = NULL;
	this->lru_block = NULL;
	this->dirty_block = NULL;
	for ( size_t i = 0; i < DEVICE_HASH_LENGTH; i++ )
		hash_blocks[i] = NULL;
	this->sync_thread_cond = PTHREAD_COND_INITIALIZER;
	this->sync_thread_idle_cond = PTHREAD_COND_INITIALIZER;
	this->sync_thread_lock = PTHREAD_MUTEX_INITIALIZER;
	this->sync_in_transit = false;
	this->has_sync_thread = false;
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
	if ( has_sync_thread )
	{
		pthread_mutex_lock(&sync_thread_lock);
		while ( dirty_block || sync_in_transit )
			pthread_cond_wait(&sync_thread_cond, &sync_thread_lock);
		pthread_mutex_unlock(&sync_thread_lock);
		return;
	}

	while ( dirty_block )
		dirty_block->Sync();
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
