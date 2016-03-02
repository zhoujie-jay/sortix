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
 * sortix/kernel/inode.h
 * Interfaces and utility classes for implementing inodes.
 */

#ifndef INCLUDE_SORTIX_KERNEL_INODE_H
#define INCLUDE_SORTIX_KERNEL_INODE_H

#include <sys/types.h>

#include <stddef.h>
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
	virtual int statvfs(ioctx_t* ctx, struct statvfs* stvfs) = 0;
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
	virtual int utimens(ioctx_t* ctx, const struct timespec* times) = 0;
	virtual int isatty(ioctx_t* ctx) = 0;
	virtual ssize_t readdirents(ioctx_t* ctx, struct dirent* dirent,
	                            size_t size, off_t start) = 0;
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
	virtual int tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp) = 0;
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws) = 0;
	virtual int tcsetpgrp(ioctx_t* ctx, pid_t pgid) = 0;
	virtual pid_t tcgetpgrp(ioctx_t* ctx) = 0;
	virtual int settermmode(ioctx_t* ctx, unsigned mode) = 0;
	virtual int gettermmode(ioctx_t* ctx, unsigned* mode) = 0;
	virtual int poll(ioctx_t* ctx, PollNode* node) = 0;
	virtual int rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
	                        const char* newname) = 0;
	virtual Ref<Inode> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
	                          int flags) = 0;
	virtual int bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen) = 0;
	virtual int connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen) = 0;
	virtual int listen(ioctx_t* ctx, int backlog) = 0;
	virtual ssize_t recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags) = 0;
	virtual ssize_t send(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                     int flags) = 0;
	virtual int getsockopt(ioctx_t* ctx, int level, int option_name,
	                       void* option_value, size_t* option_size_ptr) = 0;
	virtual int setsockopt(ioctx_t* ctx, int level, int option_name,
	                       const void* option_value, size_t option_size) = 0;
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count) = 0;
	virtual ssize_t tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count) = 0;
	virtual int unmounted(ioctx_t* ctx) = 0;
	virtual int tcdrain(ioctx_t* ctx) = 0;
	virtual int tcflow(ioctx_t* ctx, int action) = 0;
	virtual int tcflush(ioctx_t* ctx, int queue_selector) = 0;
	virtual int tcgetattr(ioctx_t* ctx, struct termios* tio) = 0;
	virtual pid_t tcgetsid(ioctx_t* ctx) = 0;
	virtual int tcsendbreak(ioctx_t* ctx, int duration) = 0;
	virtual int tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio) = 0;

};

enum InodeType
{
	INODE_TYPE_UNKNOWN = 0,
	INODE_TYPE_FILE,
	INODE_TYPE_STREAM,
	INODE_TYPE_TTY,
	INODE_TYPE_DIR,
	INODE_TYPE_SYMLINK,
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
	struct timespec stat_atim;
	struct timespec stat_mtim;
	struct timespec stat_ctim;
	blksize_t stat_blksize;
	blkcnt_t stat_blocks;

public:
	AbstractInode();
	virtual ~AbstractInode();
	virtual void linked();
	virtual void unlinked();
	virtual int sync(ioctx_t* ctx);
	virtual int stat(ioctx_t* ctx, struct stat* st);
	virtual int statvfs(ioctx_t* ctx, struct statvfs* stvfs);
	virtual int chmod(ioctx_t* ctx, mode_t mode);
	virtual int chown(ioctx_t* ctx, uid_t owner, gid_t group);
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);
	virtual int utimens(ioctx_t* ctx, const struct timespec* times);
	virtual int isatty(ioctx_t* ctx);
	virtual ssize_t readdirents(ioctx_t* ctx, struct dirent* dirent,
	                            size_t size, off_t start);
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
	virtual int tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp);
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	virtual int tcsetpgrp(ioctx_t* ctx, pid_t pgid);
	virtual pid_t tcgetpgrp(ioctx_t* ctx);
	virtual int settermmode(ioctx_t* ctx, unsigned mode);
	virtual int gettermmode(ioctx_t* ctx, unsigned* mode);
	virtual int poll(ioctx_t* ctx, PollNode* node);
	virtual int rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
	                        const char* newname);
	virtual Ref<Inode> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
	                          int flags);
	virtual int bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	virtual int connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	virtual int listen(ioctx_t* ctx, int backlog);
	virtual ssize_t recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags);
	virtual ssize_t send(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                     int flags);
	virtual int getsockopt(ioctx_t* ctx, int level, int option_name,
	                       void* option_value, size_t* option_size_ptr);
	virtual int setsockopt(ioctx_t* ctx, int level, int option_name,
	                       const void* option_value, size_t option_size);
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count);
	virtual ssize_t tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count);
	virtual int unmounted(ioctx_t* ctx);
	virtual int tcdrain(ioctx_t* ctx);
	virtual int tcflow(ioctx_t* ctx, int action);
	virtual int tcflush(ioctx_t* ctx, int queue_selector);
	virtual int tcgetattr(ioctx_t* ctx, struct termios* tio);
	virtual pid_t tcgetsid(ioctx_t* ctx);
	virtual int tcsendbreak(ioctx_t* ctx, int duration);
	virtual int tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio);

};

} // namespace Sortix

#endif
