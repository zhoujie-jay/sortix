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

    fs/kram.h
    Kernel RAM filesystem.

*******************************************************************************/

#ifndef SORTIX_FS_KRAM_H
#define SORTIX_FS_KRAM_H

#include <sortix/kernel/inode.h>
#include <sortix/kernel/kthread.h>

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
	File(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode);
	virtual ~File();
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);

private:
	virtual int truncate_unlocked(ioctx_t* ctx, off_t length);

private:
	kthread_mutex_t filelock;
	uid_t owner;
	uid_t group;
	mode_t mode;
	size_t size;
	size_t bufsize;
	uint8_t* buf;
	size_t numlinks;

};

class Dir : public AbstractInode
{
public:
	Dir(dev_t dev, ino_t ino, uid_t owner, gid_t group, mode_t mode);
	virtual ~Dir();
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
	virtual int rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
	                        const char* newname);

private:
	size_t FindChild(const char* filename);
	bool AddChild(const char* filename, Ref<Inode> inode);
	void RemoveChild(size_t index);

private:
	kthread_mutex_t dirlock;
	size_t numchildren;
	size_t childrenlen;
	DirEntry* children;
	bool shutdown;

};

} // namespace KRAMFS
} // namespace Sortix

#endif
