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
#include <sortix/kernel/process.h>
#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sortix/stat.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

namespace Sortix {

// Flags for the various base modes to open a file in.
const int ACCESS_FLAGS = O_READ | O_WRITE | O_EXEC | O_SEARCH;

// Flags that only make sense at open time.
const int OPEN_FLAGS = O_CREATE | O_DIRECTORY | O_EXCL | O_TRUNC | O_NOFOLLOW;

// Flags that only make sense for descriptors.
const int DESCRIPTOR_FLAGS = O_APPEND | O_NONBLOCK;

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
	Ref<Descriptor> ret = from->open(ctx, dirpath, O_READ | O_DIRECTORY, 0);
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

bool Descriptor::SetFlags(int new_dflags)
{
	// TODO: Hmm, there is race condition between changing the flags here and
	//       the code that uses the flags below. We could add a lock, but that
	//       would kinda prevent concurrency on the same file descriptor. Since
	//       the chances of this becoming a problem is rather slim (but could
	//       happen!), we'll do the unsafe thing for now. (See below also)
	dflags = (dflags & ~DESCRIPTOR_FLAGS) & (new_dflags & DESCRIPTOR_FLAGS);
	return true;
}

int Descriptor::GetFlags()
{
	// TODO: The race condition also applies here if the variable can change.
	return dflags;
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
	// TODO: Possible denial-of-service attack if someone opens the file without
	//       that much rights and just syncs it a whole lot and slows down the
	//       system as a whole.
	return vnode->sync(ctx);
}

int Descriptor::stat(ioctx_t* ctx, struct stat* st)
{
	// TODO: Possible information leak if not O_READ | O_WRITE and the caller
	//       is told about the file size.
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
	if ( !(dflags & O_WRITE) )
		return errno = EPERM, -1;
	return vnode->truncate(ctx, length);
}

off_t Descriptor::lseek(ioctx_t* ctx, off_t offset, int whence)
{
	// TODO: Possible information leak to let someone without O_READ | O_WRITE
	//       seek the file and get information about data holes.
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
	if ( !(dflags & O_READ) )
		return errno = EPERM, -1;
	if ( !count ) { return 0; }
	if ( (size_t) SSIZE_MAX < count ) { count = SSIZE_MAX; }
	ctx->dflags = dflags;
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
	if ( !(dflags & O_READ) )
		return errno = EPERM, -1;
	if ( off < 0 ) { errno = EINVAL; return -1; }
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	ctx->dflags = dflags;
	return vnode->pread(ctx, buf, count, off);
}

ssize_t Descriptor::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( !(dflags & O_WRITE) )
		return errno = EPERM, -1;
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	ctx->dflags = dflags;
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
	if ( !(dflags & O_WRITE) )
		return errno = EPERM, -1;
	if ( off < 0 ) { errno = EINVAL; return -1; }
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	ctx->dflags = dflags;
	return vnode->pwrite(ctx, buf, count, off);
}

int Descriptor::utimens(ioctx_t* ctx, const struct timespec* atime,
                        const struct timespec* ctime,
                        const struct timespec* mtime)
{
	return vnode->utimens(ctx, atime, ctime, mtime);
}

int Descriptor::isatty(ioctx_t* ctx)
{
	return vnode->isatty(ctx);
}

ssize_t Descriptor::readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
                                size_t size, size_t maxcount)
{
	// TODO: COMPATIBILITY HACK: Traditionally, you can open a directory with
	//       O_RDONLY and pass it to fdopendir and then use it, which doesn't
	//       set the needed O_SEARCH flag! I think some software even do it with
	//       write permissions! Currently, we just let you search the directory
	//       if you opened with any of the O_SEARCH, O_READ or O_WRITE flags.
	//       A better solution would be to make fdopendir try to add the
	//       O_SEARCH flag to the file descriptor. Or perhaps just recheck the
	//       permissions to search (execute) the directory manually every time,
	//       though that is less pure. Unfortunately, POSIX is pretty vague on
	//       how O_SEARCH should be interpreted and most existing Unix systems
	//       such as Linux doesn't even have that flag! And how about combining
	//       it with the O_EXEC flag - POSIX allows that and it makes sense
	//       because the execute bit on directories control search permission.
	if ( !(dflags & (O_SEARCH | O_READ | O_WRITE)) )
		return errno = EPERM, -1;

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

static bool IsSaneFlagModeCombination(int flags, mode_t /*mode*/)
{
	// It doesn't make sense to pass O_CREATE or O_TRUNC when attempting to open
	// a directory. We also reject O_TRUNC | O_DIRECTORY early to prevent
	// opening a directory, attempting to truncate it, and then aborting with an
	// error because a directory was opened.
	if ( (flags & (O_CREATE | O_TRUNC)) && (flags & (O_DIRECTORY)) )
		return errno = EINVAL, false;

	return true;
}

static bool IsLastPathElement(const char* elem)
{
	while ( !(*elem == '/' || *elem == '\0') )
		elem++;
	while ( *elem == '/' )
		elem++;
	return *elem == '\0';
}

Ref<Descriptor> Descriptor::open(ioctx_t* ctx, const char* filename, int flags,
                                 mode_t mode)
{
	Process* process = CurrentProcess();
	kthread_mutex_lock(&process->idlock);
	mode &= ~process->umask;
	kthread_mutex_unlock(&process->idlock);

	// Unlike early Unix systems, the empty path does not mean the current
	// directory on Sortix, so reject it.
	if ( !filename[0] )
		return errno = ENOENT, Ref<Descriptor>();

	// Reject some non-sensical flag combinations early.
	if ( !IsSaneFlagModeCombination(flags, mode) )
		return errno = EINVAL, Ref<Descriptor>();

	Ref<Descriptor> desc(this);
	while ( filename[0] )
	{
		// Reaching a slash in the path means that the caller intended what came
		// before to be a directory, stop the open call if it isn't.
		if ( filename[0] == '/' )
		{
			if ( !S_ISDIR(desc->type) )
				return errno = ENOTDIR, Ref<Descriptor>();
			filename++;
			continue;
		}

		// Cut out the next path element from the input string.
		size_t slashpos = strcspn(filename, "/");
		char* elem = String::Substring(filename, 0, slashpos);
		if ( !elem )
			return Ref<Descriptor>();

		// Decide how to open the next element in the path.
		bool lastelem = IsLastPathElement(filename);
		int open_flags = lastelem ? flags : O_READ | O_DIRECTORY;
		mode_t open_mode = lastelem ? mode : 0;

		// Open the next element in the path.
		Ref<Descriptor> next = desc->open_elem(ctx, elem, open_flags, open_mode);
		delete[] elem;

		if ( !next )
			return Ref<Descriptor>();

		desc = next;
		filename += slashpos;
	}

	// Abort the open if the user wanted a directory but this wasn't.
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

	// Verify that at least one of the base access modes are being used.
	if ( !(flags & ACCESS_FLAGS) )
		return errno = EINVAL, Ref<Descriptor>();

	// Filter away flags that only make sense for descriptors.
	int retvnode_flags = flags & ~DESCRIPTOR_FLAGS;
	Ref<Vnode> retvnode = vnode->open(ctx, filename, retvnode_flags, mode);
	if ( !retvnode )
		return Ref<Descriptor>();

	// Filter away flags that only made sense at during the open call.
	int ret_flags = flags & ~OPEN_FLAGS;
	Ref<Descriptor> ret(new Descriptor(retvnode, ret_flags));
	if ( !ret )
		return Ref<Descriptor>();

	// Truncate the file if requested.
	// TODO: This is a bit dodgy, should this be moved to the inode open method
	//       or something? And how should error handling be done here?
	if ( (flags & O_TRUNC) && S_ISREG(ret->type) )
		if ( flags & O_WRITE )
			ret->truncate(ctx, 0);

	return ret;
}

int Descriptor::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	Process* process = CurrentProcess();
	kthread_mutex_lock(&process->idlock);
	mode &= ~process->umask;
	kthread_mutex_unlock(&process->idlock);

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

int Descriptor::rename_here(ioctx_t* ctx, Ref<Descriptor> from,
                            const char* oldpath, const char* newpath)
{
	char* olddir_elem;
	char* newdir_elem;
	Ref<Descriptor> olddir = OpenDirContainingPath(ctx, from, oldpath,
	                                               &olddir_elem);
	if ( !olddir ) return -1;

	Ref<Descriptor> newdir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                               newpath, &newdir_elem);
	if ( !newdir ) { delete[] olddir_elem; return -1; }

	int ret = newdir->vnode->rename_here(ctx, olddir->vnode, olddir_elem,
	                                     newdir_elem);

	delete[] newdir_elem;
	delete[] olddir_elem;

	return ret;
}

ssize_t Descriptor::readlink(ioctx_t* ctx, char* buf, size_t bufsize)
{
	if ( !(dflags & O_READ) )
		return errno = EPERM, -1;
	if ( (size_t) SSIZE_MAX < bufsize )
		bufsize = (size_t) SSIZE_MAX;
	return vnode->readlink(ctx, buf, bufsize);
}

int Descriptor::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	return vnode->tcgetwinsize(ctx, ws);
}

int Descriptor::tcsetpgrp(ioctx_t* ctx, pid_t pgid)
{
	return vnode->tcsetpgrp(ctx, pgid);
}

pid_t Descriptor::tcgetpgrp(ioctx_t* ctx)
{
	return vnode->tcgetpgrp(ctx);
}

int Descriptor::settermmode(ioctx_t* ctx, unsigned mode)
{
	return vnode->settermmode(ctx, mode);
}

int Descriptor::gettermmode(ioctx_t* ctx, unsigned* mode)
{
	return vnode->gettermmode(ctx, mode);
}

int Descriptor::poll(ioctx_t* ctx, PollNode* node)
{
	// TODO: Perhaps deny polling against some kind of events if this
	//       descriptor's dflags would reject doing these operations?
	return vnode->poll(ctx, node);
}

Ref<Descriptor> Descriptor::accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen, int flags)
{
	Ref<Vnode> retvnode = vnode->accept(ctx, addr, addrlen, flags);
	if ( !retvnode )
		return Ref<Descriptor>();
	return Ref<Descriptor>(new Descriptor(retvnode, O_READ | O_WRITE));
}

int Descriptor::bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen)
{
	return vnode->bind(ctx, addr, addrlen);
}

int Descriptor::connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen)
{
	return vnode->connect(ctx, addr, addrlen);
}

int Descriptor::listen(ioctx_t* ctx, int backlog)
{
	return vnode->listen(ctx, backlog);
}

ssize_t Descriptor::recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags)
{
	return vnode->recv(ctx, buf, count, flags);
}

ssize_t Descriptor::send(ioctx_t* ctx, const uint8_t* buf, size_t count, int flags)
{
	return vnode->send(ctx, buf, count, flags);
}

} // namespace Sortix
