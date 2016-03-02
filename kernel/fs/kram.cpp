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
 * fs/kram.cpp
 * Kernel RAM filesystem.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/clock.h>
#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sortix/stat.h>
#include <sortix/statvfs.h>

#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/time.h>

#include "kram.h"

namespace Sortix {
namespace KRAMFS {

static ssize_t common_tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	if ( !name )
	{
		static const char index[] = "device-path\0filesystem-type\0";
		size_t index_size = sizeof(index) - 1;
		if ( buffer && count < index_size )
			return errno = ERANGE, -1;
		if ( buffer && !ctx->copy_to_dest(buffer, &index, index_size) )
			return -1;
		return (ssize_t) index_size;
	}
	else if ( !strcmp(name, "device-path") )
	{
		const char* data = "none";
		size_t size = strlen(data);
		if ( buffer && count < size )
			return errno = ERANGE, -1;
		if ( buffer && !ctx->copy_to_dest(buffer, data, size) )
			return -1;
		return (ssize_t) size;
	}
	else if ( !strcmp(name, "filesystem-type") )
	{
		const char* data = "kram";
		size_t size = strlen(data);
		if ( buffer && count < size )
			return errno = ERANGE, -1;
		if ( buffer && !ctx->copy_to_dest(buffer, data, size) )
			return -1;
		return (ssize_t) size;
	}
	else
		return errno = ENOENT, -1;
}

int common_statvfs(ioctx_t* ctx, struct statvfs* stvfs, dev_t dev)
{
	size_t memory_used;
	size_t memory_total;
	Memory::Statistics(&memory_used, &memory_total);
	struct statvfs retstvfs;
	memset(&retstvfs, 0, sizeof(retstvfs));
	retstvfs.f_bsize = Page::Size();
	retstvfs.f_frsize = Page::Size();
	retstvfs.f_blocks = memory_total / Page::Size();
	retstvfs.f_bfree = (memory_total - memory_used) / Page::Size();
	retstvfs.f_bavail = (memory_total - memory_used) / Page::Size();
	retstvfs.f_files = 0;
	retstvfs.f_ffree = 0;
	retstvfs.f_favail = 0;
	retstvfs.f_fsid = dev;
	retstvfs.f_flag = ST_NOSUID;
	retstvfs.f_namemax = ULONG_MAX;
	if ( !ctx->copy_to_dest(stvfs, &retstvfs, sizeof(retstvfs)) )
		return -1;
	return 0;
}

File::File(InodeType inode_type, mode_t type, dev_t dev, ino_t ino, uid_t owner,
           gid_t group, mode_t mode)
{
	this->inode_type = inode_type;
	if ( !dev )
		dev = (dev_t) this;
	if ( !ino )
		ino = (ino_t) this;
	this->type = type;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_size = 0;
	this->stat_blksize = 1;
	this->dev = dev;
	this->ino = ino;
}

File::~File()
{
}

int File::truncate(ioctx_t* ctx, off_t length)
{
	int ret = fcache.truncate(ctx, length);
	if ( ret == 0 )
	{
		ScopedLock lock(&metalock);
		stat_size = fcache.GetFileSize();
		stat_mtim = Time::Get(CLOCK_REALTIME);
	}
	return ret;
}

off_t File::lseek(ioctx_t* ctx, off_t offset, int whence)
{
	return fcache.lseek(ctx, offset, whence);
}

ssize_t File::pread(ioctx_t* ctx, uint8_t* dest, size_t count, off_t off)
{
	return fcache.pread(ctx, dest, count, off);
}

ssize_t File::pwrite(ioctx_t* ctx, const uint8_t* src, size_t count, off_t off)
{
	ssize_t ret = fcache.pwrite(ctx, src, count, off);
	if ( 0 < ret )
	{
		ScopedLock lock(&metalock);
		stat_size = fcache.GetFileSize();
		stat_mtim = Time::Get(CLOCK_REALTIME);
	}
	return ret;
}

ssize_t File::readlink(ioctx_t* ctx, char* buf, size_t bufsize)
{
	if ( !S_ISLNK(type) )
		return errno = EINVAL, -1;
	if ( (size_t) SSIZE_MAX < bufsize )
		bufsize = SSIZE_MAX;
	return fcache.pread(ctx, (uint8_t*) buf, bufsize, 0);
}

ssize_t File::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	return common_tcgetblob(ctx, name, buffer, count);
}

int File::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	return common_statvfs(ctx, stvfs, dev);
}

Dir::Dir(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
{
	inode_type = INODE_TYPE_DIR;
	if ( !dev )
		dev = (dev_t) this;
	if ( !ino )
		ino = (ino_t) this;
	dir_lock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_gid = owner;
	this->stat_gid = group;
	this->type = S_IFDIR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->dev = dev;
	this->ino = ino;
	children_used = 0;
	children_length = 0;
	children = NULL;
	shut_down = false;
}

Dir::~Dir()
{
	// We must not be deleted or garbage collected if we are still used by
	// someone. In that case the deleter should either delete our children or
	// simply forget about us.
	assert(!children_used);
	delete[] children;
}

ssize_t Dir::readdirents(ioctx_t* ctx, struct dirent* dirent, size_t size,
                         off_t start)
{
	ScopedLock lock(&dir_lock);
	if ( children_used <= (uintmax_t) start )
		return 0;
	struct dirent retdirent;
	memset(&retdirent, 0, sizeof(retdirent));
	const char* name = children[start].name;
	size_t namelen = strlen(name);
	Ref<Inode> inode = children[start].inode;
	retdirent.d_reclen = sizeof(*dirent) + namelen + 1;
	retdirent.d_namlen = namelen;
	retdirent.d_ino = inode->ino;
	retdirent.d_dev = inode->dev;
	retdirent.d_type = ModeToDT(inode->type);
	if ( !ctx->copy_to_dest(dirent, &retdirent, sizeof(retdirent)) )
		return -1;
	if ( size < retdirent.d_reclen )
		return errno = ERANGE, -1;
	if ( !ctx->copy_to_dest(dirent->d_name, name, namelen+1) )
		return -1;
	return (ssize_t) retdirent.d_reclen;
}

size_t Dir::FindChild(const char* filename)
{
	for ( size_t i = 0; i < children_used; i++ )
		if ( !strcmp(filename, children[i].name) )
			return i;
	return SIZE_MAX;
}

bool Dir::AddChild(const char* filename, Ref<Inode> inode)
{
	if ( children_used == children_length )
	{
		size_t new_children_length = children_length ? 2 * children_length : 4;
		DirEntry* new_children = new DirEntry[new_children_length];
		if ( !new_children )
			return false;
		for ( size_t i = 0; i < children_used; i++ )
		{
			new_children[i].inode = children[i].inode;
			new_children[i].name = children[i].name;
			children[i].inode.Reset();
		}
		delete[] children; children = new_children;
		children_length = new_children_length;
	}
	char* filename_copy = String::Clone(filename);
	if ( !filename_copy )
		return false;
	inode->linked();
	DirEntry* dirent = &children[children_used++];
	dirent->inode = inode;
	dirent->name = filename_copy;
	return true;
}

void Dir::RemoveChild(size_t index)
{
	assert(index < children_used);
	if ( index != children_used-1 )
	{
		DirEntry tmp = children[index];
		children[index] = children[children_used-1];
		children[children_used-1] = tmp;
		index = children_used-1;
	}
	children[index].inode.Reset();
	delete[] children[index].name;
	children_used--;
}

Ref<Inode> Dir::open(ioctx_t* ctx, const char* filename, int flags, mode_t mode)
{
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, Ref<Inode>(NULL);
	size_t child_index = FindChild(filename);
	if ( child_index != SIZE_MAX )
	{
		if ( (flags & O_CREATE) && (flags & O_EXCL) )
			return errno = EEXIST, Ref<Inode>(NULL);
		return children[child_index].inode;
	}
	if ( !(flags & O_CREATE) )
		return errno = ENOENT, Ref<Inode>(NULL);
	Ref<File> file(new File(INODE_TYPE_FILE, S_IFREG, dev, 0, ctx->uid,
	                        ctx->gid, mode));
	if ( !file )
		return Ref<Inode>(NULL);
	if ( !AddChild(filename, file) )
		return Ref<Inode>(NULL);
	return file;
}

int Dir::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	size_t child_index = FindChild(filename);
	if ( child_index != SIZE_MAX )
		return errno = EEXIST, -1;
	Ref<Dir> dir(new Dir(dev, 0, ctx->uid, ctx->gid, mode));
	if ( !dir )
		goto cleanup_done;
	if ( dir->link_raw(ctx, ".", dir) )
		goto cleanup_done;
	if ( dir->link_raw(ctx, "..", Ref<Dir>(this)) )
		goto cleanup_dot;
	if ( !AddChild(filename, dir) )
		goto cleanup_dotdot;
	return 0;
cleanup_dotdot:
	dir->unlink_raw(ctx, "..");
cleanup_dot:
	dir->unlink_raw(ctx, ".");
cleanup_done:
	return -1;
}

int Dir::rmdir(ioctx_t* ctx, const char* filename)
{
	if ( IsDotOrDotDot(filename) )
		return errno = ENOTEMPTY, -1;
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	size_t child_index = FindChild(filename);
	if ( child_index == SIZE_MAX )
		return errno = ENOENT, -1;
	Inode* child = children[child_index].inode.Get();
	if ( !S_ISDIR(child->type) )
		return errno = ENOTDIR, -1;
	if ( child->rmdir_me(ctx) < 0 )
		return -1;
	RemoveChild(child_index);
	return 0;
}

int Dir::rmdir_me(ioctx_t* /*ctx*/)
{
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	for ( size_t i = 0; i < children_used; i++ )
		if ( !IsDotOrDotDot(children[i].name) )
			return errno = ENOTEMPTY, -1;
	shut_down = true;
	for ( size_t i = 0; i < children_used; i++ )
	{
		children[i].inode->unlinked();
		children[i].inode.Reset();
		delete[] children[i].name;
	}
	delete[] children; children = NULL;
	children_used = children_length = 0;
	return 0;
}

int Dir::link(ioctx_t* /*ctx*/, const char* filename, Ref<Inode> node)
{
	if ( S_ISDIR(node->type) )
		return errno = EPERM, -1;
	// TODO: Is this needed? This may protect against file descriptors to
	// deleted directories being used to corrupt kernel state, or something.
	if ( IsDotOrDotDot(filename) )
		return errno = EEXIST, -1;
	if ( node->dev != dev )
		return errno = EXDEV, -1;
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	size_t child_index = FindChild(filename);
	if ( child_index != SIZE_MAX )
		return errno = EEXIST, -1;
	if ( !AddChild(filename, node) )
		return -1;
	return 0;
}

int Dir::link_raw(ioctx_t* /*ctx*/, const char* filename, Ref<Inode> node)
{
	if ( node->dev != dev )
		return errno = EXDEV, -1;
	ScopedLock lock(&dir_lock);
	size_t child_index = FindChild(filename);
	if ( child_index != SIZE_MAX )
	{
		children[child_index].inode->unlinked();
		children[child_index].inode = node;
		children[child_index].inode->linked();
	}
	else
	{
		if ( !AddChild(filename, node) )
			return -1;
	}
	return 0;
}

int Dir::unlink(ioctx_t* /*ctx*/, const char* filename)
{
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	size_t child_index = FindChild(filename);
	if ( child_index == SIZE_MAX )
		return errno = ENOENT, -1;
	Inode* child = children[child_index].inode.Get();
	if ( S_ISDIR(child->type) )
		return errno = EISDIR, -1;
	RemoveChild(child_index);
	return 0;
}

int Dir::unlink_raw(ioctx_t* /*ctx*/, const char* filename)
{
	ScopedLock lock(&dir_lock);
	size_t child_index = FindChild(filename);
	if ( child_index == SIZE_MAX )
		return errno = ENOENT, -1;
	RemoveChild(child_index);
	return 0;
}

int Dir::symlink(ioctx_t* ctx, const char* oldname, const char* filename)
{
	ScopedLock lock(&dir_lock);
	if ( shut_down )
		return errno = ENOENT, -1;
	if ( FindChild(filename) != SIZE_MAX )
		return errno = EEXIST, -1;
	Ref<File> file(new File(INODE_TYPE_SYMLINK, S_IFLNK, dev, 0, ctx->uid,
	                        ctx->gid, 0777));
	if ( !file )
		return Ref<Inode>(NULL);
	ioctx_t kctx;
	SetupKernelIOCtx(&kctx);
	size_t oldname_length = strlen(oldname);
	size_t so_far = 0;
	while ( so_far < oldname_length )
	{
#if OFF_MAX < SIZE_MAX
		if ( (uintmax_t) OFF_MAX < (uintmax_t) so_far )
			return Ref<Inode>(NULL);
#endif
		ssize_t amount = file->pwrite(&kctx, (const uint8_t*) oldname + so_far,
		                              oldname_length - so_far, (off_t) so_far);
		if ( amount <= 0 )
			return Ref<Inode>(NULL);
		so_far += (size_t) amount;
	}
	if ( !AddChild(filename, file) )
		return Ref<Inode>(NULL);
	return 0;
}

int Dir::rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
                     const char* newname)
{
	if ( IsDotOrDotDot(oldname) || IsDotOrDotDot(newname) )
		return errno = EINVAL, -1;

	// TODO: Check whether oldpath is an ancestor of newpath.

	// Avoid deadlocks by locking directories in the right order.
	Dir* from_dir = (Dir*) from.Get();
	kthread_mutex_t* mutex_ptr1;
	kthread_mutex_t* mutex_ptr2;
	if ( from_dir->ino < this->ino )
	{
		mutex_ptr1 = &from_dir->dir_lock;
		mutex_ptr2 = &this->dir_lock;
	}
	else if ( from_dir->ino == this->ino )
	{
		mutex_ptr1 = &this->dir_lock,
		mutex_ptr2 = NULL;
		if ( !strcmp(oldname, newname) )
			return 0;
	}
	else
	{
		mutex_ptr1 = &this->dir_lock;
		mutex_ptr2 = &from_dir->dir_lock;
	}
	ScopedLock lock1(mutex_ptr1);
	ScopedLock lock2(mutex_ptr2);

	size_t from_index = from_dir->FindChild(oldname);
	if ( from_index == SIZE_MAX )
		return errno = ENOENT, -1;

	Ref<Inode> the_inode = from_dir->children[from_index].inode;

	size_t to_index = this->FindChild(newname);
	if ( to_index != SIZE_MAX )
	{
		Ref<Inode> existing = this->children[to_index].inode;

		if ( existing->dev == the_inode->dev &&
		     existing->ino == the_inode->ino )
			return 0;

		if ( S_ISDIR(existing->type) )
		{
			Dir* existing_dir = (Dir*) existing.Get();
			if ( !S_ISDIR(the_inode->type) )
				return errno = EISDIR, -1;
			assert(&existing_dir->dir_lock != mutex_ptr1);
			assert(&existing_dir->dir_lock != mutex_ptr2);
			if ( existing_dir->rmdir_me(ctx) != 0 )
				return -1;
		}
		this->children[to_index].inode = the_inode;
	}
	else
	{
		if ( !this->AddChild(newname, the_inode) )
			return -1;
	}

	from_dir->RemoveChild(from_index);

	if ( S_ISDIR(the_inode->type) )
		the_inode->link_raw(ctx, "..", Ref<Inode>(this));

	return 0;
}

ssize_t Dir::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	return common_tcgetblob(ctx, name, buffer, count);
}

int Dir::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	return common_statvfs(ctx, stvfs, dev);
}

} // namespace KRAMFS
} // namespace Sortix
