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

    sortix/kernel/vnode.h
    Nodes in the virtual filesystem.

*******************************************************************************/

#ifndef SORTIX_VNODE_H
#define SORTIX_VNODE_H

#include <sortix/kernel/refcount.h>

struct stat;
struct timeval;
struct winsize;
struct kernel_dirent;

namespace Sortix {

class Inode;
struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;

// An interface describing all operations possible on an vnode.
class Vnode : public Refcountable
{
public: /* These must never change after construction and is read-only. */
	ino_t ino;
	dev_t dev;
	mode_t type; // For use by S_IS* macros.

public:
	Vnode(Ref<Inode> inode, Ref<Vnode> mountedat, ino_t rootino, dev_t rootdev);
	virtual ~Vnode();
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
	int utimes(ioctx_t* ctx, const struct timeval times[2]);
	int isatty(ioctx_t* ctx);
	ssize_t readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
	                    size_t size, off_t start, size_t maxcount);
	Ref<Vnode> open(ioctx_t* ctx, const char* filename, int flags, mode_t mode);
	int mkdir(ioctx_t* ctx, const char* filename, mode_t mode);
	int unlink(ioctx_t* ctx, const char* filename);
	int rmdir(ioctx_t* ctx, const char* filename);
	int link(ioctx_t* ctx, const char* filename, Ref<Vnode> node);
	int symlink(ioctx_t* ctx, const char* oldname, const char* filename);
	ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	int fsbind(ioctx_t* ctx, Vnode* node, int flags);
	int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	int settermmode(ioctx_t* ctx, unsigned mode);
	int gettermmode(ioctx_t* ctx, unsigned* mode);

private:
	Ref<Inode> inode;
	Ref<Vnode> mountedat;
	ino_t rootino;
	dev_t rootdev;

};

} // namespace Sortix

#endif
