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
 * vnode.cpp
 * Nodes in the virtual filesystem.
 */

#include <assert.h>
#include <errno.h>

#include <sortix/fcntl.h>
#include <sortix/mount.h>
#include <sortix/stat.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/process.h>

#include "fs/user.h"

namespace Sortix {

static Ref<Inode> LookupMountUnlocked(Ref<Inode> inode, size_t* out_index = NULL)
{
	Ref<Inode> result_inode(NULL);
	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	for ( size_t i = 0; i < mtable->nummounts; i++ )
	{
		mountpoint_t* mp = mtable->mounts + i;
		if ( mp->ino != inode->ino || mp->dev != inode->dev )
			continue;
		if ( out_index )
			*out_index = i;
		result_inode = mp->inode;
	}
	return result_inode;
}

static Ref<Inode> LookupMount(Ref<Inode> inode)
{
	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	ScopedLock mtable_lock(&mtable->mtablelock);
	return LookupMountUnlocked(inode);
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
	bool dotdot = strcmp(filename, "..") == 0;

	// Prevent escaping the root filesystem.
	if ( dotdot )
	{
		Ref<Descriptor> root = CurrentProcess()->GetRoot();
		if ( root->ino == ino && root->dev == dev )
			return Ref<Vnode>(this);
	}

	// Handle transition across filesystem mount points.
	bool isroot = inode->ino == rootino && inode->dev == rootdev;
	if ( isroot && dotdot && mountedat )
		return mountedat;

	// Move within the current filesystem.
	Ref<Inode> retinode = inode->open(ctx, filename, flags, mode);
	if ( !retinode )
		return Ref<Vnode>(NULL);
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

int Vnode::fsm_fsbind(ioctx_t* ctx, Ref<Vnode> target, int flags)
{
	(void) ctx;
	if ( flags & ~(0) )
		return errno = EINVAL, -1;
	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	if ( !mtable->AddMount(ino, dev, target->inode, true) )
		return -1;
	return 0;
}

Ref<Vnode> Vnode::fsm_mount(ioctx_t* ctx,
                            const char* filename,
                            const struct stat* rootst_ptr,
                            int flags)
{
	if ( flags & ~(0) )
		return Ref<Vnode>(NULL);

	if ( !strcmp(filename, ".") || !strcmp(filename, "..") )
		return errno = EINVAL, Ref<Vnode>(NULL);

	struct stat rootst;
	if ( !ctx->copy_from_src(&rootst, rootst_ptr, sizeof(struct stat)) )
		return Ref<Vnode>(NULL);

	Ref<Inode> normal_inode = inode->open(ctx, filename, O_READ, 0);
	if ( !normal_inode )
		return Ref<Vnode>(NULL);

	Ref<Inode> root_inode;
	Ref<Inode> server_inode;
	if ( !UserFS::Bootstrap(&root_inode, &server_inode, &rootst) )
		return Ref<Vnode>(NULL);

	Ref<Vnode> server_vnode(new Vnode(server_inode, Ref<Vnode>(NULL), 0, 0));
	if ( !server_vnode )
		return Ref<Vnode>(NULL);

	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	ScopedLock lock(&mtable->mtablelock);
	if ( Ref<Inode> mounted = LookupMountUnlocked(normal_inode) )
		normal_inode = mounted;

	if ( !mtable->AddMountUnlocked(normal_inode->ino, normal_inode->dev, root_inode, false) )
		return Ref<Vnode>(NULL);

	return server_vnode;
}

int Vnode::unmount(ioctx_t* ctx, const char* filename, int flags)
{
	if ( flags & ~(UNMOUNT_FORCE | UNMOUNT_DETACH) )
		return errno = EINVAL, -1;

	if ( !strcmp(filename, ".") || !strcmp(filename, "..") )
		return errno = EINVAL, -1;

	Ref<Inode> normal_inode = inode->open(ctx, filename, O_READ, 0);
	if ( !normal_inode )
		return -1;

	Ref<MountTable> mtable = CurrentProcess()->GetMTable();
	ScopedLock mtable_lock(&mtable->mtablelock);

	size_t mp_index;
	if ( !LookupMountUnlocked(normal_inode, &mp_index) )
		return errno = ENOMOUNT, -1;
	mountpoint_t* mp = mtable->mounts + mp_index;

	Ref<Inode> mp_inode = mp->inode;
	bool mp_fsbind = mp->fsbind;

	mp->inode.Reset();
	for ( size_t n = mp_index; n < mtable->nummounts - 1; n++ )
	{
		mtable->mounts[n] = mtable->mounts[n+1];
		mtable->mounts[n+1].inode.Reset();
	}
	mtable->nummounts--;

	mtable_lock.Reset();

	if ( !mp_fsbind )
	{
		// TODO: Implement the !UNMOUNT_DETACH case.
		// TODO: Implement the UNMOUNT_FORCE case.
		mp_inode->unmounted(ctx);
	}
	mp_inode.Reset();

	return 0;
}

int Vnode::sync(ioctx_t* ctx)
{
	return inode->sync(ctx);
}

int Vnode::stat(ioctx_t* ctx, struct stat* st)
{
	return inode->stat(ctx, st);
}

int Vnode::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	return inode->statvfs(ctx, stvfs);
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

int Vnode::utimens(ioctx_t* ctx, const struct timespec* times)
{
	return inode->utimens(ctx, times);
}

int Vnode::isatty(ioctx_t* ctx)
{
	return inode->isatty(ctx);
}

ssize_t Vnode::readdirents(ioctx_t* ctx, struct dirent* dirent,
                           size_t size, off_t start)
{
	return inode->readdirents(ctx, dirent, size, start);
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

int Vnode::tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp)
{
	return inode->tcgetwincurpos(ctx, wcp);
}

int Vnode::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	return inode->tcgetwinsize(ctx, ws);
}

int Vnode::tcsetpgrp(ioctx_t* ctx, pid_t pgid)
{
	return inode->tcsetpgrp(ctx, pgid);
}

pid_t Vnode::tcgetpgrp(ioctx_t* ctx)
{
	return inode->tcgetpgrp(ctx);
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

int Vnode::getsockopt(ioctx_t* ctx, int level, int option_name,
                      void* option_value, size_t* option_size_ptr)
{
	return inode->getsockopt(ctx, level, option_name, option_value, option_size_ptr);
}

int Vnode::setsockopt(ioctx_t* ctx, int level, int option_name,
                      const void* option_value, size_t option_size)
{
	return inode->setsockopt(ctx, level, option_name, option_value, option_size);
}

ssize_t Vnode::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	return inode->tcgetblob(ctx, name, buffer, count);
}

ssize_t Vnode::tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count)
{
	return inode->tcsetblob(ctx, name, buffer, count);
}

int Vnode::tcdrain(ioctx_t* ctx)
{
	return inode->tcdrain(ctx);
}

int Vnode::tcflow(ioctx_t* ctx, int action)
{
	return inode->tcflow(ctx, action);
}

int Vnode::tcflush(ioctx_t* ctx, int queue_selector)
{
	return inode->tcflush(ctx, queue_selector);
}

int Vnode::tcgetattr(ioctx_t* ctx, struct termios* tio)
{
	return inode->tcgetattr(ctx, tio);
}

pid_t Vnode::tcgetsid(ioctx_t* ctx)
{
	return inode->tcgetsid(ctx);
}

int Vnode::tcsendbreak(ioctx_t* ctx, int duration)
{
	return inode->tcsendbreak(ctx, duration);
}

int Vnode::tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio)
{
	return inode->tcsetattr(ctx, actions, tio);
}

} // namespace Sortix
