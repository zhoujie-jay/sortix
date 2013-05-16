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

    sortix/kernel/descriptor.h
    A file descriptor.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_DESCRIPTOR_H
#define INCLUDE_SORTIX_KERNEL_DESCRIPTOR_H

#include <sys/types.h>

#include <stdint.h>

#include <sortix/timespec.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

struct stat;
struct winsize;
struct kernel_dirent;

namespace Sortix {

class PollNode;
class Inode;
class Vnode;
struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;

class Descriptor : public Refcountable
{
public:
	Descriptor(Ref<Vnode> vnode, int dflags);
	virtual ~Descriptor();
	Ref<Descriptor> Fork();
	bool SetFlags(int new_dflags);
	int GetFlags();
	int sync(ioctx_t* ctx);
	int stat(ioctx_t* ctx, struct stat* st);
	int chmod(ioctx_t* ctx, mode_t mode);
	int chown(ioctx_t* ctx, uid_t owner, gid_t group);
	int truncate(ioctx_t* ctx, off_t length);
	off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off);
	int utimens(ioctx_t* ctx, const struct timespec* atime,
	            const struct timespec* ctime, const struct timespec* mtime);
	int isatty(ioctx_t* ctx);
	ssize_t readdirents(ioctx_t* ctx, struct kernel_dirent* dirent, size_t size,
	                    size_t maxcount);
	Ref<Descriptor> open(ioctx_t* ctx, const char* filename, int flags,
	                     mode_t mode = 0);
	int mkdir(ioctx_t* ctx, const char* filename, mode_t mode);
	int link(ioctx_t* ctx, const char* filename, Ref<Descriptor> node);
	int unlink(ioctx_t* ctx, const char* filename);
	int rmdir(ioctx_t* ctx, const char* filename);
	int symlink(ioctx_t* ctx, const char* oldname, const char* filename);
	ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	int settermmode(ioctx_t* ctx, unsigned mode);
	int gettermmode(ioctx_t* ctx, unsigned* mode);
	int poll(ioctx_t* ctx, PollNode* node);
	int rename_here(ioctx_t* ctx, Ref<Descriptor> from, const char* oldpath,
	                const char* newpath);
	Ref<Descriptor> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
	                       int flags);
	int bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	int connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	int listen(ioctx_t* ctx, int backlog);
	ssize_t recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags);
	ssize_t send(ioctx_t* ctx, const uint8_t* buf, size_t count, int flags);

private:
	Ref<Descriptor> open_elem(ioctx_t* ctx, const char* filename, int flags,
	                          mode_t mode);
	bool IsSeekable();

public: /* These must never change after construction and is read-only. */
	ino_t ino;
	dev_t dev;
	mode_t type; // For use by S_IS* macros.

public /*TODO: private*/:
	Ref<Vnode> vnode;
	kthread_mutex_t curofflock;
	bool seekable;
	bool checked_seekable;
	off_t curoff;
	int dflags;

};

bool LinkInodeInDir(ioctx_t* ctx, Ref<Descriptor> dir, const char* name,
                    Ref<Inode> inode);
Ref<Descriptor> OpenDirContainingPath(ioctx_t* ctx, Ref<Descriptor> from,
                                      const char* path, char** finalp);

} // namespace Sortix

#endif
