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

    sortix/kernel/inode.h
    Interfaces and utility classes for implementing inodes.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_INODE_H
#define INCLUDE_SORTIX_KERNEL_INODE_H

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/timespec.h>

#include <sortix/kernel/refcount.h>

struct stat;
struct timeval;
struct winsize;
struct kernel_dirent;

namespace Sortix {

struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;

// An interface describing all operations possible on an inode.
class Inode : public Refcountable
{
public: /* These must never change after construction and is read-only. */
	ino_t ino;
	dev_t dev;
	mode_t type; // For use by S_IS* macros.

public:
	virtual ~Inode() { }
	virtual void linked() = 0;
	virtual void unlinked() = 0;
	virtual int sync(ioctx_t* ctx) = 0;
	virtual int stat(ioctx_t* ctx, struct stat* st) = 0;
	virtual int chmod(ioctx_t* ctx, mode_t mode) = 0;
	virtual int chown(ioctx_t* ctx, uid_t owner, gid_t group) = 0;
	virtual int truncate(ioctx_t* ctx, off_t length) = 0;
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence) = 0;
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count) = 0;
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off) = 0;
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count) = 0;
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off) = 0;
	virtual int utimes(ioctx_t* ctx, const struct timeval times[2]) = 0;
	virtual int isatty(ioctx_t* ctx) = 0;
	virtual ssize_t readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
	                            size_t size, off_t start, size_t maxcount) = 0;
	virtual Ref<Inode> open(ioctx_t* ctx, const char* filename, int flags,
	                        mode_t mode) = 0;
	virtual int mkdir(ioctx_t* ctx, const char* filename, mode_t mode) = 0;
	virtual int link(ioctx_t* ctx, const char* filename, Ref<Inode> node) = 0;
	virtual int link_raw(ioctx_t* ctx, const char* filename, Ref<Inode> node) = 0;
	virtual int unlink(ioctx_t* ctx, const char* filename) = 0;
	virtual int unlink_raw(ioctx_t* ctx, const char* filename) = 0;
	virtual int rmdir(ioctx_t* ctx, const char* filename) = 0;
	virtual int rmdir_me(ioctx_t* ctx) = 0;
	virtual int symlink(ioctx_t* ctx, const char* oldname,
	                    const char* filename) = 0;
	virtual ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz) = 0;
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws) = 0;
	virtual int settermmode(ioctx_t* ctx, unsigned mode) = 0;
	virtual int gettermmode(ioctx_t* ctx, unsigned* mode) = 0;

};

enum InodeType
{
	INODE_TYPE_UNKNOWN = 0,
	INODE_TYPE_FILE,
	INODE_TYPE_STREAM,
	INODE_TYPE_TTY,
	INODE_TYPE_DIR,
};

class AbstractInode : public Inode
{
protected:
	kthread_mutex_t metalock;
	InodeType inode_type;
	mode_t stat_mode;
	/*nlink_t*/ unsigned long stat_nlink;
	uid_t stat_uid;
	gid_t stat_gid;
	off_t stat_size;
	time_t stat_atime;
	time_t stat_mtime;
	time_t stat_ctime;
	/* TODO: stat_atim, stat_mtim, stat_ctim */
	blksize_t stat_blksize;
	blkcnt_t stat_blocks;

public:
	AbstractInode();
	virtual ~AbstractInode();
	virtual void linked();
	virtual void unlinked();
	virtual int sync(ioctx_t* ctx);
	virtual int stat(ioctx_t* ctx, struct stat* st);
	virtual int chmod(ioctx_t* ctx, mode_t mode);
	virtual int chown(ioctx_t* ctx, uid_t owner, gid_t group);
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);
	virtual int utimes(ioctx_t* ctx, const struct timeval times[2]);
	virtual int isatty(ioctx_t* ctx);
	virtual ssize_t readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
	                            size_t size, off_t start, size_t maxcount);
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
	virtual ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	virtual int settermmode(ioctx_t* ctx, unsigned mode);
	virtual int gettermmode(ioctx_t* ctx, unsigned* mode);

};

} // namespace Sortix

#endif