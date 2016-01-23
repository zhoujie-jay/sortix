/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014, 2015.

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

    inode.cpp
    Interfaces and utility classes for implementing inodes.

*******************************************************************************/

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <sortix/clock.h>
#include <sortix/stat.h>
#include <sortix/statvfs.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/time.h>

namespace Sortix {

AbstractInode::AbstractInode()
{
	metalock = KTHREAD_MUTEX_INITIALIZER;
	inode_type = INODE_TYPE_UNKNOWN;
	stat_mode = 0;
	stat_nlink = 0;
	stat_uid = 0;
	stat_gid = 0;
	stat_size = 0;
	stat_atim = Time::Get(CLOCK_REALTIME);
	stat_ctim = Time::Get(CLOCK_REALTIME);
	stat_mtim = Time::Get(CLOCK_REALTIME);
	stat_blksize = 0;
	stat_blocks = 0;
}

AbstractInode::~AbstractInode()
{
}

void AbstractInode::linked()
{
	InterlockedIncrement(&stat_nlink);
}

void AbstractInode::unlinked()
{
	InterlockedDecrement(&stat_nlink);
}

int AbstractInode::sync(ioctx_t* /*ctx*/)
{
	return 0;
}

int AbstractInode::stat(ioctx_t* ctx, struct stat* st)
{
	struct stat retst;
	ScopedLock lock(&metalock);
	memset(&retst, 0, sizeof(retst));
	retst.st_dev = dev;
	retst.st_rdev = dev;
	retst.st_ino = ino;
	retst.st_mode = stat_mode;
	retst.st_nlink = (nlink_t) stat_nlink;
	retst.st_uid = stat_uid;
	retst.st_gid = stat_gid;
	retst.st_size = stat_size;
	retst.st_atim = stat_atim;
	retst.st_ctim = stat_ctim;
	retst.st_mtim = stat_mtim;
	retst.st_blksize = stat_blksize;
	retst.st_blocks = stat_size / 512;
	if ( !ctx->copy_to_dest(st, &retst, sizeof(retst)) )
		return -1;
	return 0;
}

// TODO: Provide an easier mechanism for letting subclasses give this
//       information than overriding this method. Additionally, what should be
//       done in abstract kernel objects where this call doesn't make that much
//       sense?
int AbstractInode::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	struct statvfs retstvfs;
	ScopedLock lock(&metalock);
	memset(&retstvfs, 0, sizeof(retstvfs));
	retstvfs.f_bsize = 0;
	retstvfs.f_frsize = 0;
	retstvfs.f_blocks = 0;
	retstvfs.f_bfree = 0;
	retstvfs.f_bavail = 0;
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

int AbstractInode::chmod(ioctx_t* /*ctx*/, mode_t mode)
{
	ScopedLock lock(&metalock);
	stat_mode = (mode & S_SETABLE) | this->type;
	return 0;
}

int AbstractInode::chown(ioctx_t* /*ctx*/, uid_t owner, gid_t group)
{
	ScopedLock lock(&metalock);
	stat_uid = owner;
	stat_gid= group;
	return 0;
}

int AbstractInode::truncate(ioctx_t* /*ctx*/, off_t /*length*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EISDIR, -1;
	return errno = EINVAL, -1;
}

off_t AbstractInode::lseek(ioctx_t* /*ctx*/, off_t /*offset*/, int /*whence*/)
{
	if ( inode_type == INODE_TYPE_STREAM || inode_type == INODE_TYPE_TTY )
		return errno = ESPIPE, -1;
	return errno = EBADF, -1;
}

ssize_t AbstractInode::read(ioctx_t* /*ctx*/, uint8_t* /*buf*/,
                            size_t /*count*/)
{
	return errno = EBADF, -1;
}

ssize_t AbstractInode::pread(ioctx_t* /*ctx*/, uint8_t* /*buf*/,
                             size_t /*count*/, off_t /*off*/)
{
	if ( inode_type == INODE_TYPE_STREAM || inode_type == INODE_TYPE_TTY )
		return errno = ESPIPE, -1;
	return errno = EBADF, -1;
}

ssize_t AbstractInode::write(ioctx_t* /*ctx*/, const uint8_t* /*buf*/,
                             size_t /*count*/)
{
	return errno = EBADF, -1;
}

ssize_t AbstractInode::pwrite(ioctx_t* /*ctx*/, const uint8_t* /*buf*/,
                              size_t /*count*/, off_t /*off*/)
{
	if ( inode_type == INODE_TYPE_STREAM || inode_type == INODE_TYPE_TTY )
		return errno = ESPIPE, -1;
	return errno = EBADF, -1;
}

int AbstractInode::utimens(ioctx_t* /*ctx*/, const struct timespec* atime,
                           const struct timespec* ctime,
                           const struct timespec* mtime)
{
	ScopedLock lock(&metalock);
	if ( atime )
		stat_atim = *atime;
	if ( ctime )
		stat_ctim = *ctime;
	if ( mtime )
		stat_mtim = *mtime;
	return 0;
}

int AbstractInode::isatty(ioctx_t* /*ctx*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return 1;
	return errno = ENOTTY, 0;
}

ssize_t AbstractInode::readdirents(ioctx_t* /*ctx*/,
                                   struct kernel_dirent* /*dirent*/,
                                   size_t /*size*/, off_t /*start*/,
                                   size_t /*maxcount*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

Ref<Inode> AbstractInode::open(ioctx_t* /*ctx*/, const char* /*filename*/,
                               int /*flags*/, mode_t /*mode*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, Ref<Inode>(NULL);
	return errno = ENOTDIR, Ref<Inode>(NULL);
}

int AbstractInode::mkdir(ioctx_t* /*ctx*/, const char* /*filename*/,
                         mode_t /*mode*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::link(ioctx_t* /*ctx*/, const char* /*filename*/,
                        Ref<Inode> /*node*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::link_raw(ioctx_t* /*ctx*/, const char* /*filename*/,
                            Ref<Inode> /*node*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::unlink(ioctx_t* /*ctx*/, const char* /*filename*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::unlink_raw(ioctx_t* /*ctx*/, const char* /*filename*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::rmdir(ioctx_t* /*ctx*/, const char* /*filename*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::rmdir_me(ioctx_t* /*ctx*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

int AbstractInode::symlink(ioctx_t* /*ctx*/, const char* /*oldname*/,
                           const char* /*filename*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

ssize_t AbstractInode::readlink(ioctx_t* /*ctx*/, char* /*buf*/,
                                size_t /*bufsiz*/)
{
	return errno = EINVAL, -1;
}

int AbstractInode::tcgetwincurpos(ioctx_t* /*ctx*/, struct wincurpos* /*wcp*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

int AbstractInode::tcgetwinsize(ioctx_t* /*ctx*/, struct winsize* /*ws*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

int AbstractInode::tcsetpgrp(ioctx_t* /*ctx*/, pid_t /*pgid*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

pid_t AbstractInode::tcgetpgrp(ioctx_t* /*ctx*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

int AbstractInode::settermmode(ioctx_t* /*ctx*/, unsigned /*mode*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

int AbstractInode::gettermmode(ioctx_t* /*ctx*/, unsigned* /*mode*/)
{
	if ( inode_type == INODE_TYPE_TTY )
		return errno = EBADF, -1;
	return errno = ENOTTY, -1;
}

int AbstractInode::poll(ioctx_t* /*ctx*/, PollNode* /*node*/)
{
#if 0 // TODO: Support poll on regular files as per POSIX.
	if ( inode_type == INODE_TYPE_FILE )
	{
		// TODO: Correct bits?
		if ( !((POLLIN | POLLOUT) & node->events) )
			return errno = EAGAIN, -1;
		node->master->revents |= (POLLIN | POLLOUT) & node->events;
		return 0;
	}
#endif
	return errno = ENOTSUP, -1;
}

int AbstractInode::rename_here(ioctx_t* /*ctx*/, Ref<Inode> /*from*/,
                               const char* /*oldname*/, const char* /*newname*/)
{
	if ( inode_type == INODE_TYPE_DIR )
		return errno = EBADF, -1;
	return errno = ENOTDIR, -1;
}

Ref<Inode> AbstractInode::accept(ioctx_t* /*ctx*/, uint8_t* /*addr*/,
                                 size_t* /*addrlen*/, int /*flags*/)
{
	return errno = ENOTSOCK, Ref<Inode>();
}

int AbstractInode::bind(ioctx_t* /*ctx*/, const uint8_t* /*addr*/,
                        size_t /*addrlen*/)
{
	return errno = ENOTSOCK, -1;
}

int AbstractInode::connect(ioctx_t* /*ctx*/, const uint8_t* /*addr*/,
                           size_t /*addrlen*/)
{
	return errno = ENOTSOCK, -1;
}

int AbstractInode::listen(ioctx_t* /*ctx*/, int /*backlog*/)
{
	return errno = ENOTSOCK, -1;
}

ssize_t AbstractInode::recv(ioctx_t* /*ctx*/, uint8_t* /*buf*/,
                            size_t /*count*/, int /*flags*/)
{
	return errno = ENOTSOCK, -1;
}

ssize_t AbstractInode::send(ioctx_t* /*ctx*/, const uint8_t* /*buf*/,
                            size_t /*count*/, int /*flags*/)
{
	return errno = ENOTSOCK, -1;
}

int AbstractInode::getsockopt(ioctx_t* /*ctx*/, int /*level*/, int /*option_name*/,
                              void* /*option_value*/, size_t* /*option_size_ptr*/)
{
	return errno = ENOTSOCK, -1;
}

int AbstractInode::setsockopt(ioctx_t* /*ctx*/, int /*level*/, int /*option_name*/,
                              const void* /*option_value*/, size_t /*option_size*/)
{
	return errno = ENOTSOCK, -1;
}

ssize_t AbstractInode::tcgetblob(ioctx_t* /*ctx*/, const char* /*name*/, void* /*buffer*/, size_t /*count*/)
{
	return errno = ENOTTY, -1;
}

ssize_t AbstractInode::tcsetblob(ioctx_t* /*ctx*/, const char* /*name*/, const void* /*buffer*/, size_t /*count*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::unmounted(ioctx_t* /*ctx*/)
{
	return 0;
}

int AbstractInode::tcdrain(ioctx_t* /*ctx*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::tcflow(ioctx_t* /*ctx*/, int /*action*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::tcflush(ioctx_t* /*ctx*/, int /*queue_selector*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::tcgetattr(ioctx_t* /*ctx*/, struct termios* /*tio*/)
{
	return errno = ENOTTY, -1;
}

pid_t AbstractInode::tcgetsid(ioctx_t* /*ctx*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::tcsendbreak(ioctx_t* /*ctx*/, int /*duration*/)
{
	return errno = ENOTTY, -1;
}

int AbstractInode::tcsetattr(ioctx_t* /*ctx*/, int /*actions*/, const struct termios* /*tio*/)
{
	return errno = ENOTTY, -1;
}

} // namespace Sortix
