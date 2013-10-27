/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    fs/kram.cpp
    Kernel RAM filesystem.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/clock.h>
#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sortix/stat.h>

#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/time.h>

#include "kram.h"

namespace Sortix {
namespace KRAMFS {

File::File(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
{
	inode_type = INODE_TYPE_FILE;
	if ( !dev )
		dev = (dev_t) this;
	if ( !ino )
		ino = (ino_t) this;
	this->type = S_IFREG;
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

Dir::Dir(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode)
{
	inode_type = INODE_TYPE_DIR;
	if ( !dev )
		dev = (dev_t) this;
	if ( !ino )
		ino = (ino_t) this;
	dirlock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_gid = owner;
	this->stat_gid = group;
	this->type = S_IFDIR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->dev = dev;
	this->ino = ino;
	numchildren = 0;
	childrenlen = 0;
	children = NULL;
	shutdown = false;
}

Dir::~Dir()
{
	// We must not be deleted or garbage collected if we are still used by
	// someone. In that case the deleter should either delete our children or
	// simply forget about us.
	assert(!numchildren);
	delete[] children;
}

ssize_t Dir::readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
                         size_t size, off_t start, size_t /*maxcount*/)
{
	ScopedLock lock(&dirlock);
	if ( numchildren <= (uintmax_t) start )
		return 0;
	struct kernel_dirent retdirent;
	memset(&retdirent, 0, sizeof(retdirent));
	const char* name = children[start].name;
	size_t namelen = strlen(name);
	size_t needed = sizeof(*dirent) + namelen + 1;
	ssize_t ret = -1;
	if ( size < needed )
	{
		errno = ERANGE;
		retdirent.d_namelen = namelen;
	}
	else
	{
		Ref<Inode> inode = children[start].inode;
		ret = needed;
		retdirent.d_reclen = needed;
		retdirent.d_off = 0;
		retdirent.d_namelen = namelen;
		retdirent.d_ino = inode->ino;
		retdirent.d_dev = inode->dev;
		retdirent.d_type = ModeToDT(inode->type);
	}
	if ( !ctx->copy_to_dest(dirent, &retdirent, sizeof(retdirent)) )
		return -1;
	if ( 0 <= ret && !ctx->copy_to_dest(dirent->d_name, name, namelen+1) )
		return -1;
	return ret;
}

size_t Dir::FindChild(const char* filename)
{
	for ( size_t i = 0; i < numchildren; i++ )
		if ( !strcmp(filename, children[i].name) )
			return i;
	return SIZE_MAX;
}

bool Dir::AddChild(const char* filename, Ref<Inode> inode)
{
	if ( numchildren == childrenlen )
	{
		size_t newchildrenlen = childrenlen ? 2 * childrenlen : 4;
		DirEntry* newchildren = new DirEntry[newchildrenlen];
		if ( !newchildren )
			return false;
		for ( size_t i = 0; i < numchildren; i++ )
			newchildren[i].inode = children[i].inode,
			newchildren[i].name = children[i].name;
		delete[] children; children = newchildren;
		childrenlen = newchildrenlen;
	}
	char* filenamecopy = String::Clone(filename);
	if ( !filenamecopy )
		return false;
	inode->linked();
	DirEntry* dirent = children + numchildren++;
	dirent->inode = inode;
	dirent->name = filenamecopy;
	return true;
}

void Dir::RemoveChild(size_t index)
{
	assert(index < numchildren);
	if ( index != numchildren-1 )
	{
		DirEntry tmp = children[index];
		children[index] = children[numchildren-1];
		children[numchildren-1] = tmp;
		index = numchildren-1;
	}
	children[index].inode.Reset();
	delete[] children[index].name;
	numchildren--;
}

Ref<Inode> Dir::open(ioctx_t* ctx, const char* filename, int flags, mode_t mode)
{
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return Ref<Inode>(NULL); }
	size_t childindex = FindChild(filename);
	if ( childindex != SIZE_MAX )
	{
		if ( flags & O_EXCL ) { errno = EEXIST; return Ref<Inode>(NULL); }
		return children[childindex].inode;
	}
	if ( !(flags & O_CREATE) )
		return errno = ENOENT, Ref<Inode>(NULL);
	Ref<File> file(new File(dev, 0, ctx->uid, ctx->gid, mode));
	if ( !file )
		return Ref<Inode>(NULL);
	if ( !AddChild(filename, file) )
		return Ref<Inode>(NULL);
	return file;
}

int Dir::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	size_t childindex = FindChild(filename);
	if ( childindex != SIZE_MAX ) { errno = EEXIST; return -1; }
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
	if ( IsDotOrDotDot(filename) ) { errno = ENOTEMPTY; return -1; }
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	size_t childindex = FindChild(filename);
	if ( childindex == SIZE_MAX ) { errno = ENOENT; return -1; }
	Inode* child = children[childindex].inode.Get();
	if ( !S_ISDIR(child->type) ) { errno = ENOTDIR; return -1; }
	if ( child->rmdir_me(ctx) < 0 ) { return -1; }
	RemoveChild(childindex);
	return 0;
}

int Dir::rmdir_me(ioctx_t* /*ctx*/)
{
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	for ( size_t i = 0; i < numchildren; i++ )
		if ( !IsDotOrDotDot(children[i].name) )
			return errno = ENOTEMPTY, -1;
	shutdown = true;
	for ( size_t i = 0; i < numchildren; i++ )
	{
		children[i].inode->unlinked();
		children[i].inode.Reset();
		delete[] children[i].name;
	}
	delete[] children; children = NULL;
	numchildren = childrenlen = 0;
	return 0;
}

int Dir::link(ioctx_t* /*ctx*/, const char* filename, Ref<Inode> node)
{
	if ( S_ISDIR(node->type) ) { errno = EPERM; return -1; }
	// TODO: Is this needed? This may protect against file descriptors to
	// deleted directories being used to corrupt kernel state, or something.
	if ( IsDotOrDotDot(filename) ) { errno = EEXIST; return -1; }
	if ( node->dev != dev ) { errno = EXDEV; return -1; }
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	size_t childindex = FindChild(filename);
	if ( childindex != SIZE_MAX )
	{
		errno = EEXIST;
		return -1;
	}
	else
		if ( !AddChild(filename, node) )
			return -1;
	return 0;
}

int Dir::link_raw(ioctx_t* /*ctx*/, const char* filename, Ref<Inode> node)
{
	if ( node->dev != dev ) { errno = EXDEV; return -1; }
	ScopedLock lock(&dirlock);
	size_t childindex = FindChild(filename);
	if ( childindex != SIZE_MAX )
	{
		children[childindex].inode->unlinked();
		children[childindex].inode = node;
		children[childindex].inode->linked();
	}
	else
		if ( !AddChild(filename, node) )
			return -1;
	return 0;
}

int Dir::unlink(ioctx_t* /*ctx*/, const char* filename)
{
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	size_t childindex = FindChild(filename);
	if ( childindex == SIZE_MAX ) { errno = ENOENT; return -1; }
	Inode* child = children[childindex].inode.Get();
	if ( S_ISDIR(child->type) ) { errno = EISDIR; return -1; }
	RemoveChild(childindex);
	return 0;
}

int Dir::unlink_raw(ioctx_t* /*ctx*/, const char* filename)
{
	ScopedLock lock(&dirlock);
	size_t childindex = FindChild(filename);
	if ( childindex == SIZE_MAX ) { errno = ENOENT; return -1; }
	RemoveChild(childindex);
	return 0;
}

int Dir::symlink(ioctx_t* /*ctx*/, const char* oldname, const char* filename)
{
	ScopedLock lock(&dirlock);
	if ( shutdown ) { errno = ENOENT; return -1; }
	(void) oldname;
	(void) filename;
	errno = ENOSYS;
	return -1;
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
		mutex_ptr1 = &from_dir->dirlock,
		mutex_ptr2 = &this->dirlock;
	else if ( from_dir->ino == this->ino )
	{
		mutex_ptr1 = &this->dirlock,
		mutex_ptr2 = NULL;
		if ( !strcmp(oldname, newname) )
			return 0;
	}
	else
		mutex_ptr1 = &this->dirlock,
		mutex_ptr2 = &from_dir->dirlock;
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
			assert(&existing_dir->dirlock != mutex_ptr1);
			assert(&existing_dir->dirlock != mutex_ptr2);
			if ( existing_dir->rmdir_me(ctx) != 0 )
				return -1;
		}
		this->children[to_index].inode = the_inode;
	}
	else
		if ( !this->AddChild(newname, the_inode) )
			return -1;

	from_dir->RemoveChild(from_index);

	if ( S_ISDIR(the_inode->type) )
		the_inode->link_raw(ctx, "..", Ref<Inode>(this));

	return 0;
}

} // namespace KRAMFS
} // namespace Sortix
