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
 * mount/filesystem.h
 * Filesystem abstraction.
 */

#ifndef INCLUDE_MOUNT_FILESYSTEM_H
#define INCLUDE_MOUNT_FILESYSTEM_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <time.h>

#include <mount/error.h>

struct blockdevice;
struct filesystem;
struct filesystem_handler;

#define FILESYSTEM_FLAG_UUID (1 << 0)
#define FILESYSTEM_FLAG_FSCK_SHOULD (1 << 1)
#define FILESYSTEM_FLAG_FSCK_MUST (1 << 2)

struct filesystem
{
	struct blockdevice* bdev;
	const struct filesystem_handler* handler;
	void* handler_private;
	const char* fstype_name;
	const char* fsck;
	const char* driver;
	int flags;
	unsigned char uuid[16];
	void* user_ctx;
};

struct filesystem_handler
{
	const char* handler_name;
	size_t (*probe_amount)(struct blockdevice*);
	bool (*probe)(struct blockdevice*, const unsigned char*, size_t);
	enum filesystem_error (*inspect)(struct filesystem**, struct blockdevice*);
	void (*release)(struct filesystem*);
};

#if defined(__cplusplus)
extern "C" {
#endif

const char* filesystem_error_string(enum filesystem_error);
enum filesystem_error
blockdevice_inspect_filesystem(struct filesystem**, struct blockdevice*);
void filesystem_release(struct filesystem*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
