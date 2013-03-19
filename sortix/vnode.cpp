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

    vnode.cpp
    Nodes in the virtual filesystem.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/mtable.h>
#include <sortix/mount.h>
#include <assert.h>
#include <errno.h>
#include "process.h"

namespace Sortix {

static Ref<Inode> LookupMount(Ref<Inode> inode)
{
	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	ScopedLock lock(&mtable->mtablelock);
	for ( size_t i = 0; i < mtable->nummounts; i++ )
	{
		mountpoint_t* mp = mtable->mounts + i;
		if ( mp->ino != inode->ino || mp->dev != inode->dev )
			continue;
		return mp->inode;
	}
	return Ref<Inode>(NULL);
}

Vnode::Vnode(Ref<Inode> inode, Ref<Vnode> mountedat, ino_t rootino, dev_t rootdev)
{
	for ( Ref<Vnode> tmp = mountedat; tmp; tmp = tmp->mountedat )
		assert(tmp != this);
	this->inode = inode;
	this->mountedat = mountedat;
	this->rootino = rootino;
	this->rootdev = rootdev;
	this->ino = inode->ino;
	this->dev = inode->dev;
	this->type = inode->type;
}

Vnode::~Vnode()
{
}

Ref<Vnode> Vnode::open(ioctx_t* ctx, const char* filename, int flags, mode_t mode)
{
	// Handle transition across filesystem mount points.
	bool isroot = inode->ino == rootino && inode->dev == rootdev;
	bool dotdot = strcmp(filename, "..") == 0;
	if ( isroot && dotdot && mountedat )
		return mountedat;

	// Move within the current filesystem.
	Ref<Inode> retinode = inode->open(ctx, filename, flags, mode);
	if ( !retinode ) { return Ref<Vnode>(NULL); }
	Ref<Vnode> retmountedat = mountedat;
	ino_t retrootino = rootino;
	dev_t retrootdev = rootdev;

	// Check whether we moved into a filesystem mount point.
	Ref<Inode> mounted = LookupMount(retinode);
	if ( mounted )
	{
		retinode = mounted;
		retmountedat = Ref<Vnode>(this);
		retrootino = mounted->ino;
		retrootdev = mounted->dev;
	}

	return Ref<Vnode>(new Vnode(retinode, retmountedat, retrootino, retrootdev));
}

int Vnode::sync(ioctx_t* ctx)
{
	return inode->sync(ctx);
}

int Vnode::stat(ioctx_t* ctx, struct stat* st)
{
	return inode->stat(ctx, st);
}

int Vnode::chmod(ioctx_t* ctx, mode_t mode)
{
	return inode->chmod(ctx, mode);
}

int Vnode::chown(ioctx_t* ctx, uid_t owner, gid_t group)
{
	return inode->chown(ctx, owner, group);
}

int Vnode::truncate(ioctx_t* ctx, off_t length)
{
	return inode->truncate(ctx, length);
}

off_t Vnode::lseek(ioctx_t* ctx, off_t offset, int whence)
{
	return inode->lseek(ctx, offset, whence);
}

ssize_t Vnode::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	return inode->read(ctx, buf, count);
}

ssize_t Vnode::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	return inode->pread(ctx, buf, count, off);
}

ssize_t Vnode::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	return inode->write(ctx, buf, count);
}

ssize_t Vnode::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off)
{
	return inode->pwrite(ctx, buf, count, off);
}

int Vnode::utimens(ioctx_t* ctx, const struct timespec times[2])
{
	return inode->utimens(ctx, times);
}

int Vnode::isatty(ioctx_t* ctx)
{
	return inode->isatty(ctx);
}

ssize_t Vnode::readdirents(ioctx_t* ctx, struct kernel_dirent* dirent,
                           size_t size, off_t start, size_t count)
{
	return inode->readdirents(ctx, dirent, size, start, count);
}

int Vnode::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	return inode->mkdir(ctx, filename, mode);
}

int Vnode::unlink(ioctx_t* ctx, const char* filename)
{
	return inode->unlink(ctx, filename);
}

int Vnode::rmdir(ioctx_t* ctx, const char* filename)
{
	return inode->rmdir(ctx, filename);
}

int Vnode::link(ioctx_t* ctx, const char* filename, Ref<Vnode> node)
{
	if ( node->inode->dev != inode->dev ) { errno = EXDEV; return -1; }
	return inode->link(ctx, filename, node->inode);
}

int Vnode::symlink(ioctx_t* ctx, const char* oldname, const char* filename)
{
	return inode->symlink(ctx, oldname, filename);
}

int Vnode::rename_here(ioctx_t* ctx, Ref<Vnode> from, const char* oldname,
                       const char* newname)
{
	if ( from->dev != dev )
		return errno = EXDEV, -1;
	// TODO: Force the same mount point here, like Linux does.
	return inode->rename_here(ctx, from->inode, oldname, newname);
}

ssize_t Vnode::readlink(ioctx_t* ctx, char* buf, size_t bufsiz)
{
	return inode->readlink(ctx, buf, bufsiz);
}

int Vnode::fsbind(ioctx_t* /*ctx*/, Vnode* /*node*/, int /*flags*/)
{
	// TODO: Support binding in namespaces.
	errno = ENOSYS;
	return -1;
}

int Vnode::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	return inode->tcgetwinsize(ctx, ws);
}

int Vnode::settermmode(ioctx_t* ctx, unsigned mode)
{
	return inode->settermmode(ctx, mode);
}

int Vnode::gettermmode(ioctx_t* ctx, unsigned* mode)
{
	return inode->gettermmode(ctx, mode);
}

int Vnode::poll(ioctx_t* ctx, PollNode* node)
{
	return inode->poll(ctx, node);
}

Ref<Vnode> Vnode::accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen, int flags)
{
	Ref<Inode> retinode = inode->accept(ctx, addr, addrlen, flags);
	if ( !retinode )
		return Ref<Vnode>();
	return Ref<Vnode>(new Vnode(retinode, Ref<Vnode>(), retinode->ino, retinode->dev));
}

int Vnode::bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen)
{
	return inode->bind(ctx, addr, addrlen);
}

int Vnode::connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen)
{
	return inode->connect(ctx, addr, addrlen);
}

int Vnode::listen(ioctx_t* ctx, int backlog)
{
	return inode->listen(ctx, backlog);
}

ssize_t Vnode::recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags)
{
	return inode->recv(ctx, buf, count, flags);
}

ssize_t Vnode::send(ioctx_t* ctx, const uint8_t* buf, size_t count, int flags)
{
	return inode->send(ctx, buf, count, flags);
}

} // namespace Sortix
