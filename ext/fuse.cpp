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
 * fuse.cpp
 * FUSE frontend.
 */

#if !defined(__sortix__)

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "blockgroup.h"
#include "block.h"
#include "device.h"
#include "extfs.h"
#include "filesystem.h"
#include "fuse.h"
#include "inode.h"

struct ext2_fuse_ctx
{
	Device* dev;
	Filesystem* fs;
};

#ifndef S_SETABLE
#define S_SETABLE 02777
#endif

#define FUSE_FS (((struct ext2_fuse_ctx*) (fuse_get_context()->private_data))->fs)

void* ext2_fuse_init(struct fuse_conn_info* /*conn*/)
{
	return fuse_get_context()->private_data;
}

void ext2_fuse_destroy(void* fs_private)
{
	struct ext2_fuse_ctx* ext2_fuse_ctx = (struct ext2_fuse_ctx*) fs_private;
	while ( ext2_fuse_ctx->fs->mru_inode )
	{
		Inode* inode = ext2_fuse_ctx->fs->mru_inode;
		if ( inode->remote_reference_count )
			inode->RemoteUnref();
		else if ( inode->reference_count )
			inode->Unref();
	}
	ext2_fuse_ctx->fs->Sync();
	ext2_fuse_ctx->dev->Sync();
	delete ext2_fuse_ctx->fs; ext2_fuse_ctx->fs = NULL;
	delete ext2_fuse_ctx->dev; ext2_fuse_ctx->dev = NULL;
}

Inode* ext2_fuse_resolve_path(const char* path)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode(EXT2_ROOT_INO);
	if ( !inode )
		return (Inode*) NULL;
	while ( path[0] )
	{
		if ( *path == '/' )
		{
			if ( !EXT2_S_ISDIR(inode->Mode()) )
				return inode->Unref(), errno = ENOTDIR, (Inode*) NULL;
			path++;
			continue;
		}
		size_t elem_len = strcspn(path, "/");
		char* elem = strndup(path, elem_len);
		if ( !elem )
			return inode->Unref(), errno = ENOTDIR, (Inode*) NULL;
		path += elem_len;
		Inode* next = inode->Open(elem, O_RDONLY, 0);
		free(elem);
		inode->Unref();
		if ( !next )
			return NULL;
		inode = next;
	}
	return inode;
}

// Assumes that the path doesn't end with / unless it's the root directory.
Inode* ext2_fuse_parent_dir(const char** path_ptr)
{
	const char* path = *path_ptr;
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode(EXT2_ROOT_INO);
	if ( !inode )
		return (Inode*) NULL;
	while ( strchr(path, '/') )
	{
		if ( *path == '/' )
		{
			if ( !EXT2_S_ISDIR(inode->Mode()) )
				return inode->Unref(), errno = ENOTDIR, (Inode*) NULL;
			path++;
			continue;
		}
		size_t elem_len = strcspn(path, "/");
		char* elem = strndup(path, elem_len);
		if ( !elem )
			return inode->Unref(), errno = ENOTDIR, (Inode*) NULL;
		path += elem_len;
		Inode* next = inode->Open(elem, O_RDONLY, 0);
		free(elem);
		inode->Unref();
		if ( !next )
			return (Inode*) NULL;
		inode = next;
	}
	*path_ptr = *path ? path : ".";
	assert(!strchr(*path_ptr, '/'));
	return inode;
}

int ext2_fuse_getattr(const char* path, struct stat* st)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	StatInode(inode, st);
	inode->Unref();
	return 0;
}

int ext2_fuse_fgetattr(const char* /*path*/, struct stat* st,
                       struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	StatInode(inode, st);
	inode->Unref();
	return 0;
}

int ext2_fuse_readlink(const char* path, char* buf, size_t bufsize)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !EXT2_S_ISLNK(inode->Mode()) )
		return inode->Unref(), -(errno = EINVAL);
	if ( !bufsize )
		return inode->Unref(), -(errno = EINVAL);
	ssize_t amount = inode->ReadAt((uint8_t*) buf, bufsize, 0);
	if ( amount < 0 )
		return inode->Unref(), -errno;
	buf[(size_t) amount < bufsize ? (size_t) amount : bufsize - 1] = '\0';
	inode->Unref();
	return 0;
}

int ext2_fuse_mknod(const char* path, mode_t mode, dev_t dev)
{
	(void) path;
	(void) mode;
	(void) dev;
	return -(errno = ENOSYS);
}

int ext2_fuse_mkdir(const char* path, mode_t mode)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	Inode* newdir = inode->CreateDirectory(path, ExtModeFromHostMode(mode));
	inode->Unref();
	if ( !newdir )
		return -errno;
	newdir->Unref();
	return 0;
}

int ext2_fuse_unlink(const char* path)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	bool success = inode->Unlink(path, false);
	inode->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_rmdir(const char* path)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	bool success = inode->RemoveDirectory(path);
	inode->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_symlink(const char* oldname, const char* newname)
{
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return -errno;
	bool success = newdir->Symlink(newname, oldname);
	newdir->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_rename(const char* oldname, const char* newname)
{
	Inode* olddir = ext2_fuse_parent_dir(&oldname);
	if ( !olddir )
		return -errno;
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return olddir->Unref(), -errno;
	bool success = newdir->Rename(olddir, oldname, newname);
	newdir->Unref();
	olddir->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_link(const char* oldname, const char* newname)
{
	Inode* inode = ext2_fuse_resolve_path(oldname);
	if ( !inode )
		return -errno;
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return inode->Unref(), -errno;
	bool success = inode->Link(newname, inode, false);
	newdir->Unref();
	inode->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_chmod(const char* path, mode_t mode)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !FUSE_FS->device->write )
		return inode->Unref(), -(errno = EROFS);
	uint32_t req_mode = ExtModeFromHostMode(mode);
	uint32_t old_mode = inode->Mode();
	uint32_t new_mode = (old_mode & ~S_SETABLE) | (req_mode & S_SETABLE);
	inode->SetMode(new_mode);
	inode->Unref();
	return 0;
}

int ext2_fuse_chown(const char* path, uid_t owner, gid_t group)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !FUSE_FS->device->write )
		return inode->Unref(), -(errno = EROFS);
	inode->SetUserId((uint32_t) owner);
	inode->SetGroupId((uint32_t) group);
	inode->Unref();
	return 0;
}

int ext2_fuse_truncate(const char* path, off_t size)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !FUSE_FS->device->write )
		return inode->Unref(), -(errno = EROFS);
	inode->Truncate((uint64_t) size);
	inode->Unref();
	return 0;
}

int ext2_fuse_ftruncate(const char* /*path*/, off_t size,
                        struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	if ( !FUSE_FS->device->write )
		return inode->Unref(), -(errno = EROFS);
	inode->Truncate((uint64_t) size);
	inode->Unref();
	return 0;
}

int ext2_fuse_open(const char* path, struct fuse_file_info* fi)
{
	int flags = fi->flags;
	Inode* dir = ext2_fuse_parent_dir(&path);
	if ( !dir )
		return -errno;
	Inode* result = dir->Open(path, flags, 0);
	dir->Unref();
	if ( !result )
		return -errno;
	fi->fh = (uint64_t) result->inode_id;
	fi->keep_cache = 1;
	result->RemoteRefer();
	result->Unref();
	return 0;
}

int ext2_fuse_access(const char* path, int mode)
{
	Inode* dir = ext2_fuse_parent_dir(&path);
	if ( !dir )
		return -errno;
	Inode* result = dir->Open(path, O_RDONLY, 0);
	dir->Unref();
	if ( !result )
		return -errno;
	(void) mode;
	result->Unref();
	return 0;
}

int ext2_fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	int flags = fi->flags | O_CREAT;
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	Inode* result = inode->Open(path, flags, ExtModeFromHostMode(mode));
	inode->Unref();
	if ( !result )
		return -errno;
	fi->fh = (uint64_t) result->inode_id;
	fi->keep_cache = 1;
	result->RemoteRefer();
	result->Unref();
	return 0;
}

int ext2_fuse_opendir(const char* path, struct fuse_file_info* fi)
{
	return ext2_fuse_open(path, fi);
}

int ext2_fuse_read(const char* /*path*/, char* buf, size_t count, off_t offset,
                   struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	if ( INT_MAX < count )
		count = INT_MAX;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	ssize_t result = inode->ReadAt((uint8_t*) buf, count, offset);
	inode->Unref();
	return 0 <= result ? (int) result : -errno;
}

int ext2_fuse_write(const char* /*path*/, const char* buf, size_t count,
                    off_t offset, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	if ( INT_MAX < count )
		count = INT_MAX;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	ssize_t result = inode->WriteAt((const uint8_t*) buf, count, offset);
	inode->Unref();
	return 0 <= result ? (int) result : -errno;
}

int ext2_fuse_statfs(const char* /*path*/, struct statvfs* stvfs)
{
	memset(stvfs, 0, sizeof(*stvfs));
	Filesystem* fs = FUSE_FS;
	stvfs->f_bsize = fs->block_size;
	stvfs->f_frsize = fs->block_size;
	stvfs->f_blocks = fs->num_blocks;
	stvfs->f_bfree = fs->sb->s_free_blocks_count;
	stvfs->f_bavail = fs->sb->s_free_blocks_count;
	stvfs->f_files = fs->num_inodes;
	stvfs->f_ffree = fs->sb->s_free_inodes_count;
	stvfs->f_favail = fs->sb->s_free_inodes_count;
	stvfs->f_ffree = fs->sb->s_free_inodes_count;
	stvfs->f_fsid = 0;
	stvfs->f_flag = 0;
	if ( !fs->device->write )
		stvfs->f_flag |= ST_RDONLY;
	stvfs->f_namemax = 255;
	return 0;
}

int ext2_fuse_flush(const char* /*path*/, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->Sync();
	inode->Unref();
	return 0;
}

int ext2_fuse_release(const char* /*path*/, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->RemoteUnref();
	inode->Unref();
	return 0;
}

int ext2_fuse_releasedir(const char* path, struct fuse_file_info* fi)
{
	return ext2_fuse_release(path, fi);
}

int ext2_fuse_fsync(const char* /*path*/, int data, struct fuse_file_info* fi)
{
	(void) data;
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->Sync();
	inode->Unref();
	return 0;
}

/*int ext2_fuse_syncdir(const char* path, int data, struct fuse_file_info* fi)
{
	return ext2_fuse_sync(path, data, fi);
}*/

/*int ext2_fuse_setxattr(const char *, const char *, const char *, size_t, int)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_getxattr(const char *, const char *, char *, size_t)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_listxattr(const char *, char *, size_t)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_removexattr(const char *, const char *)
{
	return -(errno = ENOSYS);
}*/

int ext2_fuse_readdir(const char* /*path*/, void* buf, fuse_fill_dir_t filler,
                      off_t rec_num, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	if ( !S_ISDIR(inode->Mode()) )
		return inode->Unref(), -(errno = ENOTDIR);

	uint64_t file_size = inode->Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	while ( offset < file_size )
	{
		uint64_t entry_block_id = offset / fs->block_size;
		uint64_t entry_block_offset = offset % fs->block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = inode->GetBlock(block_id = entry_block_id)) )
			return inode->Unref(), -errno;
		const uint8_t* block_data = block->block_data + entry_block_offset;
		const struct ext_dirent* entry = (const struct ext_dirent*) block_data;
		if ( entry->inode && entry->name_len && (!rec_num || !rec_num--) )
		{
			char* entry_name = strndup(entry->name, entry->name_len);
			if ( !entry_name )
				return block->Unref(), inode->Unref(), -errno;
			memcpy(entry_name, entry->name, entry->name_len);
			bool full = filler(buf, entry_name, NULL, 0);
			free(entry_name);
			if ( full )
			{
				block->Unref();
				inode->Unref();
				return 0;
			}
		}
		offset += entry->reclen;
	}
	if ( block )
		block->Unref();

	inode->Unref();
	return 0;
}

/*int ext2_fuse_lock(const char*, struct fuse_file_info*, int, struct flock*)
{
	return -(errno = ENOSYS);
}*/

int ext2_fuse_utimens(const char* path, const struct timespec tv[2])
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !FUSE_FS->device->write )
		return inode->Unref(), -(errno = EROFS);
	inode->BeginWrite();
	inode->data->i_atime = tv[0].tv_sec;
	inode->data->i_mtime = tv[1].tv_sec;
	inode->FinishWrite();
	inode->Unref();
	return 0;
}

/*int ext2_fuse_bmap(const char*, size_t blocksize, uint64_t* idx)
{
	return -(errno = ENOSYS);
}*/

int ext2_fuse_main(const char* argv0,
                   const char* mount_path,
                   bool foreground,
                   Filesystem* fs,
                   Device* dev)
{
	struct fuse_operations operations;
	memset(&operations, 0, sizeof(operations));

	operations.access = ext2_fuse_access;
	operations.chmod = ext2_fuse_chmod;
	operations.chown = ext2_fuse_chown;
	operations.create = ext2_fuse_create;
	operations.destroy = ext2_fuse_destroy;
	operations.fgetattr = ext2_fuse_fgetattr;
	operations.flush = ext2_fuse_flush;
	operations.fsync = ext2_fuse_fsync;
	operations.ftruncate = ext2_fuse_ftruncate;
	operations.getattr = ext2_fuse_getattr;
	operations.init = ext2_fuse_init;
	operations.link = ext2_fuse_link;
	operations.mkdir = ext2_fuse_mkdir;
	operations.mknod = ext2_fuse_mknod;
	operations.opendir = ext2_fuse_opendir;
	operations.open = ext2_fuse_open;
	operations.readdir = ext2_fuse_readdir;
	operations.read = ext2_fuse_read;
	operations.readlink = ext2_fuse_readlink;
	operations.releasedir = ext2_fuse_releasedir;
	operations.release = ext2_fuse_release;
	operations.rename = ext2_fuse_rename;
	operations.rmdir = ext2_fuse_rmdir;
	operations.statfs = ext2_fuse_statfs;
	operations.symlink = ext2_fuse_symlink;
	operations.truncate = ext2_fuse_truncate;
	operations.unlink = ext2_fuse_unlink;
	operations.utimens = ext2_fuse_utimens;
	operations.write = ext2_fuse_write;

	operations.flag_nullpath_ok = 1;
	operations.flag_nopath = 1;

	char* argv_fuse[] =
	{
		(char*) argv0,
		(char*) "-s",
		(char*) mount_path,
		(char*) NULL,
	};

	int argc_fuse = sizeof(argv_fuse) / sizeof(argv_fuse[0]) - 1;

	struct ext2_fuse_ctx ext2_fuse_ctx;
	ext2_fuse_ctx.fs = fs;
	ext2_fuse_ctx.dev = dev;

	(void) foreground;

	return fuse_main(argc_fuse, argv_fuse, &operations, &ext2_fuse_ctx);
}

#endif
