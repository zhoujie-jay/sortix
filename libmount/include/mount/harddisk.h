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
 * mount/harddisk.h
 * Harddisk abstraction.
 */

#ifndef INCLUDE_MOUNT_HARDDISK_H
#define INCLUDE_MOUNT_HARDDISK_H

#include <sys/stat.h>
#include <sys/types.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stdint.h>

#include <mount/blockdevice.h>

struct harddisk
{
	struct blockdevice bdev;
	struct stat st;
	int fd;
	char* path;
	char* driver;
	char* model;
	char* serial;
	blksize_t logical_block_size;
	uint16_t cylinders;
	uint16_t heads;
	uint16_t sectors;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct harddisk* harddisk_openat(int, const char*, int);
bool harddisk_inspect_blockdevice(struct harddisk*);
void harddisk_close(struct harddisk*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
