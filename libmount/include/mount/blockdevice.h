/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * mount/blockdevice.h
 * Block device abstraction.
 */

#ifndef INCLUDE_MOUNT_BLOCKDEVICE_H
#define INCLUDE_MOUNT_BLOCKDEVICE_H

#include <sys/stat.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>

#include <mount/error.h>

struct filesystem;
struct harddisk;
struct partition;
struct partition_table;

struct blockdevice
{
	struct harddisk* hd;
	struct partition* p;
	struct partition_table* pt;
	struct filesystem* fs;
	enum partition_error pt_error;
	enum filesystem_error fs_error;
	void* user_ctx;
};

#if defined(__cplusplus)
extern "C" {
#endif

blksize_t blockdevice_logical_block_size(const struct blockdevice*);
bool blockdevice_check_reasonable_block_size(blksize_t);
off_t blockdevice_size(const struct blockdevice*);
size_t blockdevice_preadall(const struct blockdevice*, void*, size_t, off_t);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
