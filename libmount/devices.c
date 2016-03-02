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
 * devices.c
 * Locate block devices.
 */

#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <mount/devices.h>
#include <mount/harddisk.h>

static bool is_partition_name(const char* path)
{
	const char* name = path;
	for ( size_t i = 0; path[i]; i++ )
		if ( path[i] == '/' )
			name = path + i + 1;
	if ( !isalpha((unsigned char) *name) )
		return false;
	name++;
	while ( isalpha((unsigned char) *name) )
		name++;
	if ( !isdigit((unsigned char) *name) )
		return false;
	name++;
	while ( isdigit((unsigned char) *name) )
		name++;
	if ( *name != 'p' )
		return false;
	name++;
	if ( !isdigit((unsigned char) *name) )
		return false;
	name++;
	while ( isdigit((unsigned char) *name) )
		name++;
	return *name == '\0';
}

bool devices_iterate_path(bool (*callback)(void*, const char*), void* ctx)
{
	const char* dev_path = "/dev";
	DIR* dir = opendir(dev_path);
	if ( !dir )
		return errno == ENOENT;
	// TODO: This readdir loop can be derailed if the callback creates devices.
	struct dirent* entry;
	while ( (errno = 0, entry = readdir(dir)) )
	{
		if ( entry->d_name[0] == '.' )
			continue;
		if ( entry->d_type != DT_UNKNOWN && entry->d_type != DT_BLK )
			continue;
		// TODO: Remove this once partitions identify themselves as such and
		//       libmount doesn't allow using them as a real full block device.
		if ( is_partition_name(entry->d_name) )
			continue;
		char* path;
		if ( asprintf(&path, "%s/%s", dev_path, entry->d_name) < 0 )
			return closedir(dir), false;
		bool success = callback(ctx, path);
		free(path);
		if ( !success )
			return closedir(dir), false;
	}
	bool success = errno == 0;
	closedir(dir);
	return success;
}

struct devices_iterate_open
{
	void* ctx;
	bool (*callback)(void*, struct harddisk*);
};

static bool devices_iterate_open_callback(void* ctx_ptr, const char* path)
{
	struct devices_iterate_open* ctx = (struct devices_iterate_open*) ctx_ptr;
	struct harddisk* hd = harddisk_openat(AT_FDCWD, path, O_RDWR);
	if ( !hd )
	{
		int true_errno = errno;
		struct stat st;
		if ( lstat(path, 0) == 0 && !S_ISBLK(st.st_mode) )
			return true;
		errno = true_errno;
		return false;
	}
	if ( !harddisk_inspect_blockdevice(hd) )
	{
		bool success = errno == ENOTBLK;
		harddisk_close(hd);
		return success;
	}
	return ctx->callback(ctx->ctx, hd);
}

bool devices_iterate_open(bool (*callback)(void*, struct harddisk*), void* ctx)
{
	struct devices_iterate_open myctx;
	myctx.ctx = ctx;
	myctx.callback = callback;
	return devices_iterate_path(devices_iterate_open_callback, &myctx);
}

struct devices_open_all
{
	struct harddisk** hds;
	size_t hds_count;
	size_t hds_allocated;
};

static bool devices_open_all_callback(void* ctx_ptr, struct harddisk* hd)
{
	struct devices_open_all* ctx = (struct devices_open_all*) ctx_ptr;
	if ( ctx->hds_count == ctx->hds_allocated )
	{
		size_t new_allocated = ctx->hds_allocated ? 2 * ctx->hds_allocated : 16;
		struct harddisk** new_hds = (struct harddisk**)
			reallocarray(ctx->hds, new_allocated, sizeof(struct harddisk*));
		if ( !new_hds )
			return harddisk_close(hd), false;
		ctx->hds = new_hds;
		ctx->hds_allocated = new_allocated;
	}
	ctx->hds[ctx->hds_count++] = hd;
	return true;
}

bool devices_open_all(struct harddisk*** hds_ptr, size_t* hds_count_ptr)
{
	struct devices_open_all myctx;
	myctx.hds = NULL;
	myctx.hds_count = 0;
	myctx.hds_allocated = 0;
	if ( !devices_iterate_open(devices_open_all_callback, &myctx) )
	{
		for ( size_t i = 0; i < myctx.hds_count; i++ )
			harddisk_close(myctx.hds[i]);
		free(myctx.hds);
		return false;
	}
	*hds_ptr = myctx.hds;
	*hds_count_ptr = myctx.hds_count;
	return true;
}
