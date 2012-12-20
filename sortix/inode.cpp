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

    inode.cpp
    Interfaces and utility classes for implementing inodes.

*******************************************************************************/

#include <errno.h>
#include <string.h>

#include <sortix/stat.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>

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
	stat_atime = 0;
	stat_mtime = 0;
	stat_ctime = 0;
	/* TODO: stat_atim, stat_mtim, stat_ctim */
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
	retst.st_ino = ino;
	retst.st_mode = stat_mode;
	retst.st_nlink = (nlink_t) stat_nlink;
	retst.st_uid = stat_uid;
	retst.st_gid = stat_gid;
	retst.st_size = stat_size;
	// TODO: Keep track of time.
	retst.st_blksize = stat_blksize;
	retst.st_blocks = stat_size / 512;
	if ( !ctx->copy_to_dest(st, &retst, sizeof(retst)) )
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

int AbstractInode::utimes(ioctx_t* /*ctx*/, const struct timeval /*times*/[2])
{
	// TODO: Implement this!
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

int AbstractInode::tcgetwinsize(ioctx_t* /*ctx*/, struct winsize* /*ws*/)
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
		node->revents |= (POLLIN | POLLOUT) & node->events;
		// TODO: What if not listening on events (POLLIN | POLLOUT)?
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

} // namespace Sortix
