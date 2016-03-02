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
 * sortix/kernel/vnode.h
 * Nodes in the virtual filesystem.
 */

#ifndef INCLUDE_SORTIX_KERNEL_VNODE_H
#define INCLUDE_SORTIX_KERNEL_VNODE_H

#include <sys/types.h>

#include <stdint.h>

#include <sortix/timespec.h>

#include <sortix/kernel/refcount.h>

struct dirent;
struct stat;
struct statvfs;
struct termios;
struct wincurpos;
struct winsize;

namespace Sortix {

class PollNode;
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
	int statvfs(ioctx_t* ctx, struct statvfs* stvfs);
	int chmod(ioctx_t* ctx, mode_t mode);
	int chown(ioctx_t* ctx, uid_t owner, gid_t group);
	int truncate(ioctx_t* ctx, off_t length);
	off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off);
	int utimens(ioctx_t* ctx, const struct timespec* times);
	int isatty(ioctx_t* ctx);
	ssize_t readdirents(ioctx_t* ctx, struct dirent* dirent, size_t size,
	                    off_t start);
	Ref<Vnode> open(ioctx_t* ctx, const char* filename, int flags, mode_t mode);
	int mkdir(ioctx_t* ctx, const char* filename, mode_t mode);
	int unlink(ioctx_t* ctx, const char* filename);
	int rmdir(ioctx_t* ctx, const char* filename);
	int link(ioctx_t* ctx, const char* filename, Ref<Vnode> node);
	int symlink(ioctx_t* ctx, const char* oldname, const char* filename);
	ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	int fsbind(ioctx_t* ctx, Vnode* node, int flags);
	int tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp);
	int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	int tcsetpgrp(ioctx_t* ctx, pid_t pgid);
	pid_t tcgetpgrp(ioctx_t* ctx);
	int settermmode(ioctx_t* ctx, unsigned mode);
	int gettermmode(ioctx_t* ctx, unsigned* mode);
	int poll(ioctx_t* ctx, PollNode* node);
	int rename_here(ioctx_t* ctx, Ref<Vnode> from, const char* oldname,
	                const char* newname);
	Ref<Vnode> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen, int flags);
	int bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	int connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	int listen(ioctx_t* ctx, int backlog);
	ssize_t recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags);
	ssize_t send(ioctx_t* ctx, const uint8_t* buf, size_t count, int flags);
	int getsockopt(ioctx_t* ctx, int level, int option_name,
	               void* option_value, size_t* option_size_ptr);
	int setsockopt(ioctx_t* ctx, int level, int option_name,
	               const void* option_value, size_t option_size);
	ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count);
	ssize_t tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count);
	int unmount(ioctx_t* ctx, const char* filename, int flags);
	int fsm_fsbind(ioctx_t* ctx, Ref<Vnode> target, int flags);
	Ref<Vnode> fsm_mount(ioctx_t* ctx, const char* filename, const struct stat* rootst, int flags);
	int tcdrain(ioctx_t* ctx);
	int tcflow(ioctx_t* ctx, int action);
	int tcflush(ioctx_t* ctx, int queue_selector);
	int tcgetattr(ioctx_t* ctx, struct termios* tio);
	pid_t tcgetsid(ioctx_t* ctx);
	int tcsendbreak(ioctx_t* ctx, int duration);
	int tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio);

public /*TODO: private*/:
	Ref<Inode> inode;
	Ref<Vnode> mountedat;
	ino_t rootino;
	dev_t rootdev;

};

} // namespace Sortix

#endif
