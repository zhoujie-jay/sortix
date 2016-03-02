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
 * device.h
 * Block device.
 */

#ifndef DEVICE_H
#define DEVICE_H

class Block;

static const size_t DEVICE_HASH_LENGTH = 1 << 16;

class Device
{
public:
	Device(int fd, const char* path, uint32_t block_size, bool write);
	~Device();

public:
	pthread_t sync_thread;
	pthread_cond_t sync_thread_cond;
	pthread_cond_t sync_thread_idle_cond;
	pthread_mutex_t sync_thread_lock;
	Block* mru_block;
	Block* lru_block;
	Block* dirty_block;
	Block* hash_blocks[DEVICE_HASH_LENGTH];
	off_t device_size;
	const char* path;
	uint32_t block_size;
	int fd;
	bool write;
	bool has_sync_thread;
	bool sync_thread_should_exit;
	bool sync_in_transit;
	size_t block_count;
	size_t block_limit;

public:
	void SpawnSyncThread();
	Block* AllocateBlock();
	Block* GetBlock(uint32_t block_id);
	Block* GetBlockZeroed(uint32_t block_id);
	Block* GetCachedBlock(uint32_t block_id);
	void Sync();
	void SyncThread();

};

#endif
