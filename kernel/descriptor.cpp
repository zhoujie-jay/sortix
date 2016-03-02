/*
 * Copyright (c) 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * descriptor.cpp
 * A file descriptor.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <fsmarshall-msg.h>
#include <stdint.h>
#include <string.h>

#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/mount.h>
#include <sortix/seek.h>
#include <sortix/stat.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/vnode.h>

namespace Sortix {

// Flags for the various base modes to open a file in.
static const int ACCESS_FLAGS = O_READ | O_WRITE | O_EXEC | O_SEARCH;

// Flags that only make sense at open time.
static const int OPEN_FLAGS = O_CREATE | O_DIRECTORY | O_EXCL | O_TRUNC |
                              O_NOFOLLOW | O_SYMLINK_NOFOLLOW | O_NOCTTY |
                              O_TTY_INIT;

// Flags that only make sense for descriptors.
static const int DESCRIPTOR_FLAGS = O_APPEND | O_NONBLOCK;

int LinkInodeInDir(ioctx_t* ctx,
                   Ref<Descriptor> dir,
                   const char* name,
                   Ref<Inode> inode)
{
	Ref<Vnode> vnode(new Vnode(inode, Ref<Vnode>(), 0, 0));
	if ( !vnode )
		return -1;
	Ref<Descriptor> desc(new Descriptor(Ref<Vnode>(vnode), 0));
	if ( !desc )
		return -1;
	return dir->link(ctx, name, desc);
}

Ref<Descriptor> OpenDirContainingPath(ioctx_t* ctx,
                                      Ref<Descriptor> from,
                                      const char* path,
                                      char** final_ptr)
{
	if ( !path[0] )
		return errno = EINVAL, Ref<Descriptor>();
	char* dirpath;
	char* final;
	if ( !SplitFinalElem(path, &dirpath, &final) )
		return Ref<Descriptor>();
	// TODO: Removing trailing slashes in final may not the right thing.
	size_t final_length = strlen(final);
	while ( final_length && final[final_length-1] == '/' )
		final[--final_length] = 0;
	// There is room for a single character as final is not the empty string.
	if ( !final_length )
	{
		final[0] = '.';
		final[1] = '\0';
	}
	if ( !dirpath[0] )
	{
		delete[] dirpath;
		*final_ptr = final;
		return from;
	}
	Ref<Descriptor> ret = from->open(ctx, dirpath, O_READ | O_DIRECTORY, 0);
	delete[] dirpath;
	if ( !ret )
		return delete[] final, Ref<Descriptor>();
	*final_ptr = final;
	return ret;
}

// TODO: Add security checks.

Descriptor::Descriptor()
{
	current_offset_lock = KTHREAD_MUTEX_INITIALIZER;
	this->vnode = Ref<Vnode>(NULL);
	this->ino = 0;
	this->dev = 0;
	this->type = 0;
	this->dflags = 0;
	checked_seekable = false;
	seekable = false /* unused */;
	current_offset = 0;
}

Descriptor::Descriptor(Ref<Vnode> vnode, int dflags)
{
	current_offset_lock = KTHREAD_MUTEX_INITIALIZER;
	this->vnode = Ref<Vnode>(NULL);
	this->ino = 0;
	this->dev = 0;
	this->type = 0;
	this->dflags = 0;
	checked_seekable = false;
	seekable = false /* unused */;
	current_offset = 0;
	LateConstruct(vnode, dflags);
}

void Descriptor::LateConstruct(Ref<Vnode> vnode, int dflags)
{
	this->vnode = vnode;
	this->ino = vnode->ino;
	this->dev = vnode->dev;
	this->type = vnode->type;
	this->dflags = dflags;
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
	dflags = (dflags & ~DESCRIPTOR_FLAGS) | (new_dflags & DESCRIPTOR_FLAGS);
	return true;
}

int Descriptor::GetFlags()
{
	// TODO: The race condition also applies here if the variable can change.
	return dflags;
}

Ref<Descriptor> Descriptor::Fork()
{
	ScopedLock lock(&current_offset_lock);
	Ref<Descriptor> ret(new Descriptor(vnode, dflags));
	if ( !ret )
		return Ref<Descriptor>();
	ret->current_offset = current_offset;
	ret->checked_seekable = checked_seekable;
	ret->seekable = seekable;
	return ret;
}

bool Descriptor::IsSeekable()
{
	ScopedLock lock(&current_offset_lock);
	if ( !checked_seekable )
	{
		// TODO: Is this enough? Check that errno happens to be ESPIPE?
		int saved_errno = errno;
		ioctx_t ctx; SetupKernelIOCtx(&ctx);
		seekable = S_ISDIR(vnode->type) || 0 <= vnode->lseek(&ctx, SEEK_SET, 0);
		checked_seekable = true;
		errno = saved_errno;
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

int Descriptor::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	// TODO: Possible information leak if not O_READ | O_WRITE and the caller
	//       is told about the file size.
	return vnode->statvfs(ctx, stvfs);
}

int Descriptor::chmod(ioctx_t* ctx, mode_t mode)
{
	return vnode->chmod(ctx, mode);
}

int Descriptor::chown(ioctx_t* ctx, uid_t owner, gid_t group)
{
	return vnode->chown(ctx, owner, group);
}

int Descriptor::truncate(ioctx_t* ctx, off_t length)
{
	if ( length < 0 )
		return errno = EINVAL, -1;
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

	ScopedLock lock(&current_offset_lock);
	off_t reloff;
	if ( whence == SEEK_SET )
		reloff = 0;
	else if ( whence == SEEK_CUR )
		reloff = current_offset;
	else if ( whence == SEEK_END )
	{
		if ( (reloff = vnode->lseek(ctx, offset, SEEK_END)) < 0 )
			return -1;
	}
	else
		return errno = EINVAL, -1;

	if ( offset < 0 && reloff + offset < 0 )
		return errno = EOVERFLOW, -1;
	if ( OFF_MAX - current_offset < offset )
		return errno = EOVERFLOW, -1;

	return current_offset = reloff + offset;
}

ssize_t Descriptor::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	if ( !(dflags & O_READ) )
		return errno = EPERM, -1;
	if ( !count )
		return 0;
	if ( SIZE_MAX < count )
		count = SSIZE_MAX;
	ctx->dflags = dflags;
	if ( !IsSeekable() )
		return vnode->read(ctx, buf, count);
	ScopedLock lock(&current_offset_lock);
	ssize_t ret = vnode->pread(ctx, buf, count, current_offset);
	if ( 0 <= ret )
		current_offset += ret;
	return ret;
}

ssize_t Descriptor::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	if ( !(dflags & O_READ) )
		return errno = EPERM, -1;
	if ( off < 0 )
		return errno = EINVAL, -1;
	if ( !count )
		return 0;
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	ctx->dflags = dflags;
	return vnode->pread(ctx, buf, count, off);
}

ssize_t Descriptor::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( !(dflags & O_WRITE) )
		return errno = EPERM, -1;
	if ( !count )
		return 0;
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	ctx->dflags = dflags;
	if ( !IsSeekable() )
		return vnode->write(ctx, buf, count);
	ScopedLock lock(&current_offset_lock);
	if ( dflags & O_APPEND )
	{
		off_t end = vnode->lseek(ctx, 0, SEEK_END);
		if ( end < 0 )
			return -1;
		current_offset = end;
	}
	ssize_t ret = vnode->pwrite(ctx, buf, count, current_offset);
	if ( 0 <= ret )
		current_offset += ret;
	return ret;
}

ssize_t Descriptor::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off)
{
	if ( !(dflags & O_WRITE) )
		return errno = EPERM, -1;
	if ( off < 0 )
		return errno = EINVAL, -1;
	if ( !count )
		return 0;
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	ctx->dflags = dflags;
	return vnode->pwrite(ctx, buf, count, off);
}

static inline bool valid_utimens_timespec(struct timespec ts)
{
	return ts.tv_nsec < 1000000000 ||
	       ts.tv_nsec == UTIME_NOW ||
	       ts.tv_nsec == UTIME_OMIT;
}

int Descriptor::utimens(ioctx_t* ctx, const struct timespec* user_times)
{
	struct timespec times[2];
	if ( !user_times )
	{
		times[0].tv_sec = 0;
		times[0].tv_nsec = UTIME_NOW;
		times[1].tv_sec = 0;
		times[1].tv_nsec = UTIME_NOW;
	}
	else if ( !ctx->copy_from_src(&times, user_times, sizeof(times)) )
		return -1;
	if ( !valid_utimens_timespec(times[0]) ||
	     !valid_utimens_timespec(times[1]) )
		return errno = EINVAL;
	return vnode->utimens(ctx, times);
}

int Descriptor::isatty(ioctx_t* ctx)
{
	return vnode->isatty(ctx);
}

ssize_t Descriptor::readdirents(ioctx_t* ctx,
                                struct dirent* dirent,
                                size_t size)
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
	if ( SSIZE_MAX < size )
		size = SSIZE_MAX;
	if ( size < sizeof(*dirent) )
		return errno = EINVAL, -1;
	ScopedLock lock(&current_offset_lock);
	ssize_t ret = vnode->readdirents(ctx, dirent, size, current_offset);
	if ( 0 < ret )
		current_offset++;
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

	if ( !filename[0] )
		return errno = ENOENT, Ref<Descriptor>();

	// Reject some non-sensical flag combinations early.
	if ( !IsSaneFlagModeCombination(flags, mode) )
		return errno = EINVAL, Ref<Descriptor>();

	char* filename_mine = NULL;

	size_t symlink_iteration = 0;
	const size_t MAX_SYMLINK_ITERATION = 20;

	Ref<Descriptor> desc(this);
	while ( filename[0] )
	{
		// Reaching a slash in the path means that the caller intended what came
		// before to be a directory, stop the open call if it isn't.
		if ( filename[0] == '/' )
		{
			if ( !S_ISDIR(desc->type) )
				return delete[] filename_mine, errno = ENOTDIR, Ref<Descriptor>();
			filename++;
			continue;
		}

		// Cut out the next path element from the input string.
		size_t slashpos = strcspn(filename, "/");
		char* elem = String::Substring(filename, 0, slashpos);
		if ( !elem )
			return delete[] filename_mine, Ref<Descriptor>();

		// Decide how to open the next element in the path.
		bool lastelem = IsLastPathElement(filename);
		int open_flags = lastelem ? flags : O_READ | O_DIRECTORY;
		mode_t open_mode = lastelem ? mode : 0;

		// Open the next element in the path.
		Ref<Descriptor> next = desc->open_elem(ctx, elem, open_flags, open_mode);
		delete[] elem;

		if ( !next )
			return delete[] filename_mine, Ref<Descriptor>();

		filename += slashpos;

		bool want_the_symlink_itself = lastelem && (flags & O_SYMLINK_NOFOLLOW);
		if ( S_ISLNK(next->type) && !want_the_symlink_itself )
		{
			if ( (flags & O_NOFOLLOW) && lastelem )
				return delete[] filename_mine, errno = ELOOP, Ref<Descriptor>();

			if ( symlink_iteration++ == MAX_SYMLINK_ITERATION )
				return delete[] filename_mine, errno = ELOOP, Ref<Descriptor>();

			ioctx_t kctx;
			SetupKernelIOCtx(&kctx);

			struct stat st;
			if ( next->stat(&kctx, &st) < 0 )
				return delete[] filename_mine, Ref<Descriptor>();
			assert(0 <= st.st_size);

			if ( (uintmax_t) SIZE_MAX <= (uintmax_t) st.st_size )
				return delete[] filename_mine, Ref<Descriptor>();

			size_t linkpath_length = (size_t) st.st_size;
			char* linkpath = new char[linkpath_length + 1];
			if ( !linkpath )
				return delete[] filename_mine, Ref<Descriptor>();

			ssize_t linkpath_ret = next->readlink(&kctx, linkpath, linkpath_length);
			if ( linkpath_ret < 0 )
				return delete[] linkpath, delete[] filename_mine, Ref<Descriptor>();
			linkpath[linkpath_length] = '\0';

			linkpath_length = strlen(linkpath);
			if ( linkpath_length == 0 )
				return delete[] linkpath, delete[] filename_mine,
				       errno = ENOENT, Ref<Descriptor>();
			bool link_from_root = linkpath[0] == '/';

			// Either filename is the empty string or starts with a slash.
			size_t filename_length = strlen(filename);
			// TODO: Avoid overflow here.
			size_t new_filename_length = linkpath_length + filename_length;
			char* new_filename = new char[new_filename_length + 1];
			if ( !new_filename )
				return delete[] linkpath, delete[] filename_mine,
			           errno = ENOENT, Ref<Descriptor>();
			stpcpy(stpcpy(new_filename, linkpath), filename);
			delete[] filename_mine;
			filename = filename_mine = new_filename;

			if ( link_from_root )
				desc = CurrentProcess()->GetRoot();

			continue;
		}

		desc = next;
	}

	delete[] filename_mine;

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

int Descriptor::unlinkat(ioctx_t* ctx, const char* filename, int flags)
{
	if ( flags & ~(AT_REMOVEFILE | AT_REMOVEDIR) )
		return errno = EINVAL, -1;
	if ( !(flags & (AT_REMOVEFILE | AT_REMOVEDIR)) )
		flags |= AT_REMOVEFILE;
	char* final;
	Ref<Descriptor> dir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                            filename, &final);
	if ( !dir )
		return -1;
	int ret = -1;
	if ( ret < 0 && (flags & AT_REMOVEFILE) )
		ret = dir->vnode->unlink(ctx, final);
	if ( ret < 0 && (flags & AT_REMOVEDIR) )
		ret = dir->vnode->rmdir(ctx, final);
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
	if ( !olddir )
		return -1;

	Ref<Descriptor> newdir = OpenDirContainingPath(ctx, Ref<Descriptor>(this),
	                                               newpath, &newdir_elem);
	if ( !newdir )
		return delete[] olddir_elem, -1;

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
	if ( SSIZE_MAX < bufsize )
		bufsize = SSIZE_MAX;
	return vnode->readlink(ctx, buf, bufsize);
}

int Descriptor::tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp)
{
	return vnode->tcgetwincurpos(ctx, wcp);
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

int Descriptor::getsockopt(ioctx_t* ctx, int level, int option_name,
                           void* option_value, size_t* option_size_ptr)
{
	return vnode->getsockopt(ctx, level, option_name, option_value, option_size_ptr);
}

int Descriptor::setsockopt(ioctx_t* ctx, int level, int option_name,
                           const void* option_value, size_t option_size)
{
	return vnode->setsockopt(ctx, level, option_name, option_value, option_size);
}

ssize_t Descriptor::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	if ( name && !name[0] )
		name = NULL;
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	return vnode->tcgetblob(ctx, name, buffer, count);
}

ssize_t Descriptor::tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count)
{
	if ( name && !name[0] )
		name = NULL;
	if ( SSIZE_MAX < count )
		return errno = EFBIG, -1;
	return vnode->tcsetblob(ctx, name, buffer, count);
}

int Descriptor::unmount(ioctx_t* ctx, const char* filename, int flags)
{
	if ( flags & ~(UNMOUNT_FORCE | UNMOUNT_DETACH | UNMOUNT_NOFOLLOW) )
		return errno = EINVAL, -1;
	int subflags = flags & ~(UNMOUNT_NOFOLLOW);
	char* final;
	// TODO: This may follow a symlink when not supposed to!
	Ref<Descriptor> dir =
		OpenDirContainingPath(ctx, Ref<Descriptor>(this), filename, &final);
	if ( !dir )
		return -1;
	if ( !(flags & UNMOUNT_NOFOLLOW) )
	{
		// TODO: Potentially follow a symlink here!
	}
	int ret = dir->vnode->unmount(ctx, final, subflags);
	delete[] final;
	return ret;
}

int Descriptor::fsm_fsbind(ioctx_t* ctx, Ref<Descriptor> target, int flags)
{
	return vnode->fsm_fsbind(ctx, target->vnode, flags);
}

Ref<Descriptor> Descriptor::fsm_mount(ioctx_t* ctx,
                                      const char* filename,
                                      const struct stat* rootst,
                                      int flags)
{
	if ( flags & ~(FSM_MOUNT_NOFOLLOW | FSM_MOUNT_NONBLOCK) )
		return errno = EINVAL, Ref<Descriptor>(NULL);
	int result_dflags = O_READ | O_WRITE;
	if ( flags & FSM_MOUNT_NOFOLLOW ) result_dflags |= O_NONBLOCK;
	int subflags = flags & ~(FSM_MOUNT_NOFOLLOW | FSM_MOUNT_NONBLOCK);
	char* final;
	// TODO: This may follow a symlink when not supposed to!
	Ref<Descriptor> dir =
		OpenDirContainingPath(ctx, Ref<Descriptor>(this), filename, &final);
	if ( !dir )
		return errno = EINVAL, Ref<Descriptor>(NULL);
	if ( !(flags & FSM_MOUNT_NOFOLLOW) )
	{
		// TODO: Potentially follow a symlink here!
	}
	Ref<Descriptor> result(new Descriptor());
	if ( !result )
		return Ref<Descriptor>(NULL);
	Ref<Vnode> result_vnode = dir->vnode->fsm_mount(ctx, final, rootst, subflags);
	delete[] final;
	if ( !result_vnode )
		return Ref<Descriptor>(NULL);
	result->LateConstruct(result_vnode, result_dflags);
	return result;
}

int Descriptor::tcdrain(ioctx_t* ctx)
{
	return vnode->tcdrain(ctx);
}

int Descriptor::tcflow(ioctx_t* ctx, int action)
{
	return vnode->tcflow(ctx, action);
}

int Descriptor::tcflush(ioctx_t* ctx, int queue_selector)
{
	return vnode->tcflush(ctx, queue_selector);
}

int Descriptor::tcgetattr(ioctx_t* ctx, struct termios* tio)
{
	return vnode->tcgetattr(ctx, tio);
}

pid_t Descriptor::tcgetsid(ioctx_t* ctx)
{
	return vnode->tcgetsid(ctx);
}

int Descriptor::tcsendbreak(ioctx_t* ctx, int duration)
{
	return vnode->tcsendbreak(ctx, duration);
}

int Descriptor::tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio)
{
	return vnode->tcsetattr(ctx, actions, tio);
}

} // namespace Sortix
