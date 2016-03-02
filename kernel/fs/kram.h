/*
 * Copyright (c) 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * fs/kram.h
 * Kernel RAM filesystem.
 */

#ifndef SORTIX_FS_KRAM_H
#define SORTIX_FS_KRAM_H

#include <sortix/kernel/inode.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/fcache.h>

namespace Sortix {
namespace KRAMFS {

struct DirEntry
{
	Ref<Inode> inode;
	char* name;
};

class File : public AbstractInode
{
public:
	File(InodeType inode_type, mode_t type, dev_t dev, ino_t ino, uid_t owner,
         gid_t group, mode_t mode);
	virtual ~File();
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);
	virtual ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer,
	                          size_t count);
	virtual int statvfs(ioctx_t* ctx, struct statvfs* stvfs);

private:
	FileCache fcache;

};

class Dir : public AbstractInode
{
public:
	Dir(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode);
	virtual ~Dir();
	virtual ssize_t readdirents(ioctx_t* ctx, struct dirent* dirent,
	                            size_t size, off_t start);
	virtual Ref<Inode> open(ioctx_t* ctx, const char* filename, int flags,
	                        mode_t mode);
	virtual int mkdir(ioctx_t* ctx, const char* filename, mode_t mode);
	virtual int link(ioctx_t* ctx, const char* filename, Ref<Inode> node);
	virtual int link_raw(ioctx_t* ctx, const char* filename, Ref<Inode> node);
	virtual int unlink(ioctx_t* ctx, const char* filename);
	virtual int unlink_raw(ioctx_t* ctx, const char* filename);
	virtual int rmdir(ioctx_t* ctx, const char* filename);
	virtual int rmdir_me(ioctx_t* ctx);
	virtual int symlink(ioctx_t* ctx, const char* oldname,
	                    const char* filename);
	virtual int rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
	                        const char* newname);
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer,
	                          size_t count);
	virtual int statvfs(ioctx_t* ctx, struct statvfs* stvfs);

private:
	size_t FindChild(const char* filename);
	bool AddChild(const char* filename, Ref<Inode> inode);
	void RemoveChild(size_t index);

private:
	kthread_mutex_t dir_lock;
	size_t children_used;
	size_t children_length;
	DirEntry* children;
	bool shut_down;

};

} // namespace KRAMFS
} // namespace Sortix

#endif
