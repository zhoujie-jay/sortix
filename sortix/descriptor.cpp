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

    descriptor.cpp
    A file descriptor.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/copy.h> // DEBUG
#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sortix/stat.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "process.h"

namespace Sortix {

bool LinkInodeInDir(ioctx_t* ctx, Ref<Descriptor> dir, const char* name,
                    Ref<Inode> inode)
{
	Ref<Vnode> vnode(new Vnode(inode, Ref<Vnode>(), 0, 0));
	if ( !vnode ) return false;
	Ref<Descriptor> desc(new Descriptor(Ref<Vnode>(vnode), 0));
	if ( !desc ) return false;
	return dir->link(ctx, name, desc) != 0;
}

Ref<Descriptor> OpenDirContainingPath(ioctx_t* ctx, Ref<Descriptor> from,
                                      const char* path, char** finalp)
{
	if ( !path[0] ) { errno = EINVAL; return Ref<Descriptor>(); }
	char* dirpath;
	char* final;
	if ( !SplitFinalElem(path, &dirpath, &final) )
		return Ref<Descriptor>();
	// TODO: Removing trailing slashes in final may not the right thing.
	size_t finallen = strlen(final);
	while ( finallen && final[finallen-1] == '/' )
		final[--finallen] = 0;
	// Safe against buffer overflow because final contains at least one
	// character because we reject the empty string above.
	if ( !finallen )
		final[0] = '.',
		final[1] = '\0';
	if ( !dirpath[0] )
	{
		delete[] dirpath;
		*finalp = final;
		return from;
	}
	Ref<Descriptor> ret = from->open(ctx, dirpath, O_RDONLY | O_DIRECTORY, 0);
	delete[] dirpath;
	if ( !ret ) { delete[] final; return Ref<Descriptor>(); }
	*finalp = final;
	return ret;
}

// TODO: Add security checks.

Descriptor::Descriptor(Ref<Vnode> vnode, int dflags)
{
	curofflock = KTHREAD_MUTEX_INITIALIZER;
	this->vnode = vnode;
	this->ino = vnode->ino;
	this->dev = vnode->dev;
	this->type = vnode->type;
	this->dflags = dflags;
	checked_seekable = false;
	curoff = 0;
}

Descriptor::~Descriptor()
{
}

Ref<Descriptor> Descriptor::Fork()
{
	Ref<Descriptor> ret(new Descriptor(vnode, dflags));
	if ( !ret )
		return Ref<Descriptor>();
	ret->curoff = curoff;
	ret->checked_seekable = checked_seekable;
	ret->seekable = seekable;
	return ret;
}

bool Descriptor::IsSeekable()
{
	if ( !checked_seekable )
	{
		// TODO: Is this enough? Check that errno happens to be ESPIPE?
		ioctx_t ctx; SetupKernelIOCtx(&ctx);
		seekable = 0 <= vnode->lseek(&ctx, SEEK_SET, 0) || S_ISDIR(vnode->type);
		checked_seekable = true;
	}
	return seekable;
}

int Descriptor::sync(ioctx_t* ctx)
{
	return vnode->sync(ctx);
}

int Descriptor::stat(ioctx_t* ctx, struct stat* st)
{
	return vnode->stat(ctx, st);
}

int Descriptor::chmod(ioctx_t* ctx, mode_t mode)
{
	return vnode->chmod(ctx, mode);
}

int Descriptor::chown(ioctx_t* ctx, uid_t owner, gid_t group)
{
	if ( owner < 0 || group < 0 ) { errno = EINVAL; return -1; }
	return vnode->chown(ctx, owner, group);
}

int Descriptor::truncate(ioctx_t* ctx, off_t length)
{
	if ( length < 0 ) { errno = EINVAL; return -1; }
	return vnode->truncate(ctx, length);
}

off_t Descriptor::lseek(ioctx_t* ctx, off_t offset, int whence)
{
	if ( !IsSeekable() )
		return vnode->lseek(ctx, offset, whence);
	ScopedLock lock(&curofflock);
	off_t reloff;
	if ( whence == SEEK_SET )
		reloff = 0;
	else if ( whence == SEEK_CUR )
		reloff = curoff;
	else if ( whence == SEEK_END )
	{
		if ( (reloff = vnode->lseek(ctx, offset, SEEK_END)) < 0 )
			return -1;
	}
	else
		return errno = EINVAL, -1;

	if ( offset < 0 && reloff + offset < 0 )
		return errno = EOVERFLOW, -1;
	if ( OFF_MAX - curoff < offset )
		return errno = EOVERFLOW, -1;

	return curoff = reloff + offset;
}

ssize_t Descriptor::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	if ( !count ) { return 0; }
	if ( (size_t) SSIZE_MAX < count ) { count = SSIZE_MAX; }
	if ( !IsSeekable() )
		return vnode->read(ctx, buf, count);
	// TODO: Locking here only allows one task to read/write at once.
	ScopedLock lock(&curofflock);
	ssize_t ret = vnode->pread(ctx, buf, count, curoff);
	if ( 0 <= ret )
		curoff += ret;
	return ret;
}

ssize_t Descriptor::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	if ( off < 0 ) { errno = EINVAL; return -1; }
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	return vnode->pread(ctx, buf, count, off);
}

ssize_t Descriptor::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	if ( !IsSeekable() )
		return vnode->write(ctx, buf, count);
	// TODO: Locking here only allows one task to read/write at once.
	ScopedLock lock(&curofflock);
	// TODO: What if lseek fails? Sets curoff = -1, which we forward to vnodes
	// and we are not allowed to do that!
	if ( dflags & O_APPEND )
		curoff = vnode->lseek(ctx, 0, SEEK_END);
	ssize_t ret = vnode->pwrite(ctx, buf, count, curoff);
	if ( 0 <= ret )
		curoff += ret;
	return ret;
}

ssize_t Descriptor::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off)
{
	if ( off < 0 ) { errno = EINVAL; return -1; }
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	return vnode->pwrite(ctx, buf, count, off);
}

int Descriptor::utimes(ioctx_t* ctx, const struct timeval times[2])
{
	return vnode->utimes(ctx, times);
}

int Descriptor::isatty(ioctx_t* ctx)
{
	return vnode->isatty(ctx);
}

ssize_t Descriptor::readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
                                size_t size, size_t maxcount)
{
	if ( !maxcount ) { return 0; }
	if ( SSIZE_MAX < size ) { size = SSIZE_MAX; }
	if ( size < sizeof(*dirent) ) { errno = EINVAL; return -1; }
	// TODO: Locking here only allows one task to read/write at once.
	ScopedLock lock(&curofflock);
	ssize_t ret = vnode->readdirents(ctx, dirent, size, curoff, maxcount);
	if ( ret == 0 )
	{
		const char* name = "";
		size_t namelen = strlen(name);
		size_t needed = sizeof(*dirent) + namelen + 1;
		struct kernel_dirent retdirent;
		memset(&retdirent, 0, sizeof(retdirent));
		retdirent.d_reclen = needed;
		retdirent.d_off = 0;
		retdirent.d_namelen = namelen;
		if ( !ctx->copy_to_dest(dirent, &retdirent, sizeof(retdirent)) )
			return -1;
		if ( size < needed )
			return errno = ERANGE, -1;
		if ( !ctx->copy_to_dest(dirent->d_name, name, namelen+1) )
			return -1;
		return needed;
	}
	// TODO: Accessing data here is dangerous if it is userspace:
	if ( 0 < ret )
		for ( ; dirent; curoff++, dirent = kernel_dirent_next(dirent) );
	return ret;
}

Ref<Descriptor> Descriptor::open(ioctx_t* ctx, const char* filename, int flags,
                                 mode_t mode)
{
	if ( !filename[0] )
		return errno = ENOENT, Ref<Descriptor>();
	// O_DIRECTORY makes us only open directories. It is therefore a logical
	// error to provide flags that only makes sense for non-directory. We also
	// filter reject them early to prevent O_TRUNC | O_DIRECTORY from opening a
	// file, truncating it, and then aborting the open with an error.
	if ( (flags & (O_CREAT | O_TRUNC)) && (flags & (O_DIRECTORY)) )
		return errno = EINVAL, Ref<Descriptor>();
	Ref<Descriptor> desc(this);
	while ( filename[0] )
	{
		if ( filename[0] == '/' )
		{
			if ( !S_ISDIR(desc->type) )
				return errno = ENOTDIR, Ref<Descriptor>();
			filename++;
			continue;
		}
		size_t slashpos = strcspn(filename, "/");
		bool lastelem = filename[slashpos] == '\0';
		char* elem = String::Substring(filename, 0, slashpos);
		if ( !elem )
			return Ref<Descriptor>();
		int open_flags = lastelem ? flags : O_RDONLY;
		// Forward O_DIRECTORY so the operation can fail earlier. If it doesn't
		// fail and we get a non-directory in return, we'll handle it on exit
		// with no consequences (since opening an existing file is harmless).
		open_flags &= ~(0 /*| O_DIRECTORY*/);
		mode_t open_mode = lastelem ? mode : 0;
		// TODO: O_NOFOLLOW.
		Ref<Descriptor> next = desc->open_elem(ctx, elem, open_flags, open_mode);
		delete[] elem;
		if ( !next )
			return Ref<Descriptor>();
		desc = next;
		filename += slashpos;
	}
	if ( flags & O_DIRECTORY && !S_ISDIR(desc->type) )
		return errno = ENOTDIR, Ref<Descriptor>();

	// TODO: The new file descriptor may not be opened with the correct
	//       permissions in the below case!
	// If the path only contains slashes, we'll get outselves back, be sure to
	// get ourselves back.
	return desc == this ? Fork() : desc;
}

Ref<Descriptor> Descriptor::open_elem(ioctx_t* ctx, const char* filename,
                                      int flags, mode_t mode)
{
	assert(!strchr(filename, '/'));
	int next_flags = flags & ~(O_APPEND | O_CLOEXEC);
	Ref<Vnode> retvnode = vnode->open(ctx, filename, next_flags, mode);
	if ( !retvnode )
		return Ref<Descriptor>();
	Ref<Descriptor> ret(new Descriptor(retvnode, flags & O_APPEND));
	if ( !ret )
		return Ref<Descriptor>();
	if ( (flags & O_TRUNC) && S_ISREG(ret->type) )
		if ( (flags & O_ACCMODE) == O_WRONLY ||
		     (flags & O_ACCMODE) == O_RDWR )
			ret->truncate(ctx, 0);
	return ret;
}

int Descriptor::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = dir->vnode->mkdir(ctx, final, mode);
	delete[] final;
	return ret;
}

int Descriptor::link(ioctx_t* ctx, const char* filename, Ref<Descriptor> node)
{
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = dir->vnode->link(ctx, final, node->vnode);
	delete[] final;
	return ret;
}

int Descriptor::unlink(ioctx_t* ctx, const char* filename)
{
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = dir->vnode->unlink(ctx, final);
	delete[] final;
	return ret;
}

int Descriptor::rmdir(ioctx_t* ctx, const char* filename)
{
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = dir->vnode->rmdir(ctx, final);
	delete[] final;
	return ret;
}

int Descriptor::symlink(ioctx_t* ctx, const char* oldname, const char* filename)
{
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = dir->vnode->symlink(ctx, oldname, final);
	delete[] final;
	return ret;
}

ssize_t Descriptor::readlink(ioctx_t* ctx, char* buf, size_t bufsize)
{
	return vnode->readlink(ctx, buf, bufsize);
}

int Descriptor::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	return vnode->tcgetwinsize(ctx, ws);
}

int Descriptor::settermmode(ioctx_t* ctx, unsigned mode)
{
	return vnode->settermmode(ctx, mode);
}

int Descriptor::gettermmode(ioctx_t* ctx, unsigned* mode)
{
	return vnode->gettermmode(ctx, mode);
}

} // namespace Sortix
