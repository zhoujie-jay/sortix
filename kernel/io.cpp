/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    io.cpp
    Provides system calls for input and output.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/seek.h>
#include <sortix/socket.h>
#include <sortix/stat.h>
#include <sortix/uio.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/vnode.h>

#include "io.h"
#include "partition.h"

namespace Sortix {
namespace IO {

static Ref<Descriptor> PrepareLookup(const char** path, int dirfd = AT_FDCWD)
{
	if ( (*path)[0] == '/' )
		return CurrentProcess()->GetRoot();
	if ( dirfd == AT_FDCWD )
		return CurrentProcess()->GetCWD();
	return CurrentProcess()->GetDescriptor(dirfd);
}

static ssize_t sys_write(int fd, const void* buffer, size_t count)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	ssize_t ret = desc->write(&ctx, (const uint8_t*) buffer, count);
	return ret;
}

static ssize_t sys_pwrite(int fd, const void* buffer, size_t count, off_t off)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	ssize_t ret = desc->pwrite(&ctx, (const uint8_t*) buffer, count, off);
	return ret;
}

static ssize_t sys_read(int fd, void* buffer, size_t count)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	ssize_t ret = desc->read(&ctx, (uint8_t*) buffer, count);
	return ret;
}

static ssize_t sys_pread(int fd, void* buffer, size_t count, off_t off)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	ssize_t ret = desc->pread(&ctx, (uint8_t*) buffer, count, off);
	return ret;
}

static off_t sys_seek(int fd, off_t offset, int whence)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	off_t ret = desc->lseek(&ctx, offset, whence);
	return ret;
}

static int sys_close(int fd)
{
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	Ref<Descriptor> desc = dtable->FreeKeep(fd);
	dtable.Reset();
	if ( !desc )
		return -1;
	return desc->sync(&ctx);
}

static int sys_dup(int fd)
{
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	Ref<Descriptor> desc = dtable->Get(fd);
	return dtable->Allocate(desc, 0);
}

static int sys_dup3(int oldfd, int newfd, int flags)
{
	if ( flags & ~(O_CLOEXEC | O_CLOFORK) )
		return errno = EINVAL, -1;
	int fd_flags = 0;
	flags |= flags & O_CLOEXEC ? FD_CLOEXEC : 0;
	flags |= flags & O_CLOFORK ? FD_CLOFORK : 0;
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	return dtable->Copy(oldfd, newfd, fd_flags);
}

static int sys_dup2(int oldfd, int newfd)
{
	if ( oldfd < 0 || newfd < 0 )
		return errno = EINVAL, -1;
	int ret = sys_dup3(oldfd, newfd, 0);
	if ( ret < 0 && errno == EINVAL )
		return errno = 0, newfd;
	return ret;
}

// TODO: If this function fails the file may still have been created. Does a
// standard prohibit this and is that the wrong thing?
static int sys_openat(int dirfd, const char* path, int flags, mode_t mode)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	int fdflags = 0;
	if ( flags & O_CLOEXEC ) fdflags |= FD_CLOEXEC;
	if ( flags & O_CLOFORK ) fdflags |= FD_CLOFORK;
	flags &= ~(O_CLOEXEC | O_CLOFORK);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	Ref<Descriptor> desc = from->open(&ctx, relpath, flags, mode);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	return dtable->Allocate(desc, fdflags);
}

// TODO: This system call is replaced by openat, will be removed soon.
static int sys_open(const char* path, int flags, mode_t mode)
{
	return sys_openat(AT_FDCWD, path, flags, mode);
}

// TODO: This is a hack! Stat the file in some manner and check permissions.
static int sys_faccessat(int dirfd, const char* path, int /*mode*/, int flags)
{
	if ( flags & (AT_SYMLINK_NOFOLLOW) )
		return errno = EINVAL, -1;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int open_flags = O_READ | (flags & AT_SYMLINK_NOFOLLOW ? O_SYMLINK_NOFOLLOW : 0);
	Ref<Descriptor> desc = from->open(&ctx, relpath, open_flags);
	delete[] pathcopy;
	return desc ? 0 : -1;
}

// TODO: This system call is replaced by faccessat, will be removed soon.
static int sys_access(const char* path, int mode)
{
	return sys_faccessat(AT_FDCWD, path, mode, 0);
}

static int sys_unlinkat(int dirfd, const char* path, int flags)
{
	if ( !(flags & (AT_REMOVEFILE | AT_REMOVEDIR)) )
		flags |= AT_REMOVEFILE;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int ret = -1;
	if ( ret < 0 && (flags & AT_REMOVEFILE) )
		ret = from->unlink(&ctx, relpath);
	if ( ret < 0 && (flags & AT_REMOVEDIR) )
		ret = from->rmdir(&ctx, relpath);
	delete[] pathcopy;
	return ret;
}

// TODO: This system call is replaced by unlinkat, will be removed soon.
static int sys_unlink(const char* path)
{
	return sys_unlinkat(AT_FDCWD, path, 0);
}

static int sys_mkdirat(int dirfd, const char* path, mode_t mode)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int ret = from->mkdir(&ctx, relpath, mode);
	delete[] pathcopy;
	return ret;
}

// TODO: This system call is replaced by mkdirat, will be removed soon.
static int sys_mkdir(const char* path, mode_t mode)
{
	return sys_mkdirat(AT_FDCWD, path, mode);
}

// TODO: This system call is replaced by unlinkat, will be removed soon.
static int sys_rmdir(const char* path)
{
	return sys_unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

static int sys_truncateat(int dirfd, const char* path, off_t length)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_WRITE);
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->truncate(&ctx, length);
}

// TODO: This system call is replaced by truncateat, will be removed soon.
static int sys_truncate(const char* path, off_t length)
{
	return sys_truncateat(AT_FDCWD, path, length);
}

static int sys_ftruncate(int fd, off_t length)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->truncate(&ctx, length);
}

static int sys_fstatat(int dirfd, const char* path, struct stat* st, int flags)
{
	if ( flags & ~(AT_SYMLINK_NOFOLLOW) )
		return errno = EINVAL;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int open_flags = O_READ | (flags & AT_SYMLINK_NOFOLLOW ? O_SYMLINK_NOFOLLOW : 0);
	Ref<Descriptor> desc = from->open(&ctx, relpath, open_flags);
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->stat(&ctx, st);
}

// TODO: This system call is replaced by fstatat, will be removed soon.
static int sys_stat(const char* path, struct stat* st)
{
	return sys_fstatat(AT_FDCWD, path, st, 0);
}

static int sys_fstat(int fd, struct stat* st)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->stat(&ctx, st);
}

static int sys_fcntl(int fd, int cmd, uintptr_t arg)
{
	// Operations on the descriptor table.
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();

	// Operations on the file descriptior.
	if ( cmd == F_SETFD )
		return dtable->SetFlags(fd, (int) arg) ? 0 : -1;
	if ( cmd == F_GETFD )
		return dtable->GetFlags(fd);

	// Operations on the file description.
	if ( F_DECODE_CMD(F_DECODE_CMD_RAW(cmd)) == F_DUPFD_NUM )
	{
		int fd_flags = F_DECODE_FLAGS(F_DECODE_CMD_RAW(cmd));
		return dtable->Allocate(fd, fd_flags, (int) arg);
	}
	Ref<Descriptor> desc = dtable->Get(fd);
	if ( !desc )
		return -1;
	if ( cmd == F_SETFL )
		return desc->SetFlags((int) arg) ? 0 : -1;
	if ( cmd == F_GETFL )
		return desc->GetFlags();

	return errno = EINVAL, -1;
}

static int sys_ioctl(int fd, int cmd, void* /*ptr*/)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	int ret = -1;
	switch ( cmd )
	{
	default:
		errno = EINVAL;
		break;
	}
	return ret;
}

static ssize_t sys_readdirents(int fd, kernel_dirent* dirent, size_t size/*,
                               size_t maxcount*/)
{
	if ( size < sizeof(kernel_dirent) ) { errno = EINVAL; return -1; }
	if ( SSIZE_MAX < size )
		size = SSIZE_MAX;
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->readdirents(&ctx, dirent, size, 1 /*maxcount*/);
}

static int sys_fchdir(int fd)
{
	Process* process = CurrentProcess();
	Ref<Descriptor> desc = process->GetDescriptor(fd);
	if ( !desc )
		return -1;
	if ( !S_ISDIR(desc->type) )
		return errno = ENOTDIR, -1;
	process->SetCWD(desc);
	return 0;
}

static int sys_fchdirat(int dirfd, const char* path)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_READ | O_DIRECTORY);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	CurrentProcess()->SetCWD(desc);
	return 0;
}

// TODO: This system call is replaced by sys_fchownat, will be removed soon.
static int sys_chdir(const char* path)
{
	return sys_fchdirat(AT_FDCWD, path);
}

static int sys_fchroot(int fd)
{
	Process* process = CurrentProcess();
	Ref<Descriptor> desc = process->GetDescriptor(fd);
	if ( !desc )
		return -1;
	if ( !S_ISDIR(desc->type) )
		return errno = ENOTDIR, -1;
	process->SetRoot(desc);
	return 0;
}

static int sys_fchrootat(int dirfd, const char* path)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_READ | O_DIRECTORY);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	CurrentProcess()->SetRoot(desc);
	return 0;
}

static int sys_fchown(int fd, uid_t owner, gid_t group)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->chown(&ctx, owner, group);
}

static int sys_fchownat(int dirfd, const char* path, uid_t owner, gid_t group, int flags)
{
	if ( flags & ~(AT_SYMLINK_NOFOLLOW) )
		return errno = EINVAL, -1;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int open_flags = O_WRITE | (flags & AT_SYMLINK_NOFOLLOW ? O_SYMLINK_NOFOLLOW : 0);
	Ref<Descriptor> desc = from->open(&ctx, relpath, open_flags);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->chown(&ctx, owner, group);
}

// TODO: This system call is replaced by fchownat, will be removed soon.
static int sys_chown(const char* path, uid_t owner, gid_t group)
{
	return sys_fchownat(AT_FDCWD, path, owner, group, 0);
}

static int sys_fchmod(int fd, mode_t mode)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->chmod(&ctx, mode);
}

static int sys_fchmodat(int dirfd, const char* path, mode_t mode, int flags)
{
	if ( flags & ~(AT_SYMLINK_NOFOLLOW) )
		return errno = EINVAL, -1;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int open_flags = O_WRITE | (flags & AT_SYMLINK_NOFOLLOW ? O_SYMLINK_NOFOLLOW : 0);
	Ref<Descriptor> desc = from->open(&ctx, relpath, open_flags);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->chmod(&ctx, mode);
}

// TODO: This system call is replaced by fchmodat, will be removed soon.
static int sys_chmod(const char* path, mode_t mode)
{
	return sys_fchmodat(AT_FDCWD, path, mode, 0);
}

static int sys_futimens(int fd, const struct timespec user_times[2])
{
	struct timespec times[2];
	if ( !CopyFromUser(times, user_times, sizeof(times)) )
		return -1;
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->utimens(&ctx, &times[0], NULL, &times[1]);
}

static int sys_utimensat(int dirfd, const char* path,
                         const struct timespec user_times[2], int flags)
{
	if ( flags & ~(AT_SYMLINK_NOFOLLOW) )
		return errno = EINVAL, -1;
	struct timespec times[2];
	if ( !CopyFromUser(times, user_times, sizeof(times)) )
		return -1;
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	int open_flags = O_WRITE | (flags & AT_SYMLINK_NOFOLLOW ? O_SYMLINK_NOFOLLOW : 0);
	Ref<Descriptor> desc = from->open(&ctx, relpath, open_flags);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->utimens(&ctx, &times[0], NULL, &times[1]);
}

static int sys_linkat(int olddirfd, const char* oldpath,
                      int newdirfd, const char* newpath,
                      int flags)
{
	if ( flags & ~(AT_SYMLINK_FOLLOW) )
		return errno = EINVAL, -1;

	ioctx_t ctx; SetupUserIOCtx(&ctx);

	char* newpathcopy = GetStringFromUser(newpath);
	if ( !newpathcopy )
		return -1;
	const char* newrelpath = newpathcopy;
	Ref<Descriptor> newfrom(PrepareLookup(&newrelpath, newdirfd));
	if ( !newfrom ) { delete[] newpathcopy; return -1; }

	char* final_elem;
	Ref<Descriptor> dir = OpenDirContainingPath(&ctx, newfrom, newpathcopy,
	                                            &final_elem);
	delete[] newpathcopy;
	if ( !dir )
		return -1;

	char* oldpathcopy = GetStringFromUser(oldpath);
	if ( !oldpathcopy ) { delete[] final_elem; return -1; }
	const char* oldrelpath = oldpathcopy;
	Ref<Descriptor> oldfrom = PrepareLookup(&oldrelpath, olddirfd);
	if ( !oldfrom ) { delete[] oldpathcopy; delete[] final_elem; return -1; }

	int open_flags = O_READ | (flags & AT_SYMLINK_FOLLOW ? 0 : O_SYMLINK_NOFOLLOW);
	Ref<Descriptor> file = oldfrom->open(&ctx, oldrelpath, open_flags);
	delete[] oldpathcopy;
	if ( !file ) { delete[] final_elem; return -1; }

	int ret = dir->link(&ctx, final_elem, file);
	delete[] final_elem;
	return ret;
}

// TODO: This system call is replaced by linkat, will be removed soon.
static int sys_link(const char* oldpath, const char* newpath)
{
	return sys_linkat(AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
}

static int sys_symlinkat(const char* oldpath, int newdirfd, const char* newpath)
{
	ioctx_t ctx; SetupUserIOCtx(&ctx);

	char* newpathcopy = GetStringFromUser(newpath);
	if ( !newpathcopy )
		return -1;
	const char* newrelpath = newpathcopy;
	Ref<Descriptor> newfrom(PrepareLookup(&newrelpath, newdirfd));
	if ( !newfrom ) { delete[] newpathcopy; return -1; }

	char* final_elem;
	Ref<Descriptor> dir = OpenDirContainingPath(&ctx, newfrom, newpathcopy,
	                                            &final_elem);
	delete[] newpathcopy;
	if ( !dir )
		return -1;

	char* oldpathcopy = GetStringFromUser(oldpath);
	if ( !oldpathcopy ) { delete[] final_elem; return -1; }

	int ret = (errno = EPERM, -1);

	delete[] oldpathcopy;
	delete[] final_elem;

	return ret;
}


static int sys_settermmode(int fd, unsigned mode)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->settermmode(&ctx, mode);
}

static int sys_gettermmode(int fd, unsigned* mode)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->gettermmode(&ctx, mode);
}

static int sys_isatty(int fd)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return 0;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->isatty(&ctx);
}

static int sys_tcgetwinsize(int fd, struct winsize* ws)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->tcgetwinsize(&ctx, ws);
}

static int sys_tcsetpgrp(int fd, pid_t pgid)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->tcsetpgrp(&ctx, pgid);
}

static int sys_tcgetpgrp(int fd)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->tcgetpgrp(&ctx);
}

static int sys_renameat_inner(int olddirfd, const char* oldpath,
                              int newdirfd, const char* newpath)
{
	const char* oldrelpath = oldpath;
	Ref<Descriptor> olddir(PrepareLookup(&oldrelpath, olddirfd));
	if ( !olddir )
		return -1;

	const char* newrelpath = newpath;
	Ref<Descriptor> newdir(PrepareLookup(&newrelpath, newdirfd));
	if ( !newdir )
		return -1;

	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return newdir->rename_here(&ctx, olddir, oldrelpath, newrelpath);
}

static int sys_renameat(int olddirfd, const char* oldpath,
                        int newdirfd, const char* newpath)
{
	char* oldpathcopy = GetStringFromUser(oldpath);
	if ( !oldpathcopy ) return -1;
	char* newpathcopy = GetStringFromUser(newpath);
	if ( !newpathcopy ) { delete[] oldpathcopy; return -1; }
	int ret = sys_renameat_inner(olddirfd, oldpathcopy, newdirfd, newpathcopy);
	delete[] newpathcopy;
	delete[] oldpathcopy;
	return ret;
}

// TODO: This should probably be moved into user-space. It'd be nice if
// user-space could just open the symlink and read/write it like a regular file.
static ssize_t sys_readlinkat(int dirfd, const char* path, char* buf, size_t size)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	// TODO: Open the symbolic link, instead of what it points to!
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_READ);
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return (int) desc->readlink(&ctx, buf, size);
}

static int sys_fsync(int fd)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->sync(&ctx);
}

static int sys_accept4(int fd, void* addr, size_t* addrlen, int flags)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	int fdflags = 0;
	if ( flags & SOCK_CLOEXEC ) fdflags |= FD_CLOEXEC;
	if ( flags & SOCK_CLOFORK ) fdflags |= FD_CLOFORK;
	flags &= ~(SOCK_CLOEXEC | SOCK_CLOFORK);
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	Ref<Descriptor> conn = desc->accept(&ctx, (uint8_t*) addr, addrlen, flags);
	if ( !conn )
		return -1;
	if ( flags & SOCK_NONBLOCK )
		conn->SetFlags(conn->GetFlags() | O_NONBLOCK);
	return CurrentProcess()->GetDTable()->Allocate(conn, fdflags);
}

static int sys_bind(int fd, const void* addr, size_t addrlen)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->bind(&ctx, (const uint8_t*) addr, addrlen);
}

static int sys_connect(int fd, const void* addr, size_t addrlen)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->connect(&ctx, (const uint8_t*) addr, addrlen);
}

static int sys_listen(int fd, int backlog)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->listen(&ctx, backlog);
}

static ssize_t sys_recv(int fd, void* buffer, size_t count, int flags)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->recv(&ctx, (uint8_t*) buffer, count, flags);
}

static ssize_t sys_send(int fd, const void* buffer, size_t count, int flags)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->send(&ctx, (const uint8_t*) buffer, count, flags);
}

// TODO: We need to move these vector operations into the file descriptors or
//       inodes themselves to ensure that they are atomic. Currently these
//       operations may overlap and cause nasty bugs/race conditions when
//       multiple threads concurrently operates on a file.
// TODO: There is quite a bit of boiler plate code here. Can we do better?

static struct iovec* FetchIOV(const struct iovec* user_iov, int iovcnt)
{
	if ( iovcnt < 0 )
		return errno = EINVAL, (struct iovec*) NULL;
	struct iovec* ret = new struct iovec[iovcnt];
	if ( !ret )
		return NULL;
	if ( !CopyFromUser(ret, user_iov, sizeof(struct iovec) * (size_t) iovcnt) )
	{
		delete[] ret;
		return NULL;
	}
	return ret;
}

static ssize_t sys_readv(int fd, const struct iovec* user_iov, int iovcnt)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	struct iovec* iov = FetchIOV(user_iov, iovcnt);
	if ( !iov )
		return -1;
	ssize_t so_far = 0;
	for ( int i = 0; i < iovcnt && so_far != SSIZE_MAX; i++ )
	{
		uint8_t* buffer = (uint8_t*) iov[i].iov_base;
		size_t amount = iov[i].iov_len;
		ssize_t max_left = SSIZE_MAX - so_far;
		if ( (size_t) max_left < amount )
			amount = (size_t) max_left;
		ssize_t num_bytes = desc->read(&ctx, buffer, amount);
		if ( num_bytes < 0 )
		{
			delete[] iov;
			return so_far ? so_far : -1;
		}
		if ( num_bytes == 0 )
			break;
		so_far += num_bytes;

		// TODO: Is this the correct behavior?
		if ( (size_t) num_bytes != amount )
			break;
	}
	delete[] iov;
	return so_far;
}

static ssize_t sys_preadv(int fd, const struct iovec* user_iov, int iovcnt,
                          off_t offset)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	struct iovec* iov = FetchIOV(user_iov, iovcnt);
	if ( !iov )
		return -1;
	ssize_t so_far = 0;
	for ( int i = 0; i < iovcnt && so_far != SSIZE_MAX; i++ )
	{
		uint8_t* buffer = (uint8_t*) iov[i].iov_base;
		size_t amount = iov[i].iov_len;
		ssize_t max_left = SSIZE_MAX - so_far;
		if ( (size_t) max_left < amount )
			amount = (size_t) max_left;
		ssize_t num_bytes = desc->pread(&ctx, buffer, amount, offset + so_far);
		if ( num_bytes < 0 )
		{
			delete[] iov;
			return so_far ? so_far : -1;
		}
		if ( num_bytes == 0 )
			break;
		so_far += num_bytes;

		// TODO: Is this the correct behavior?
		if ( (size_t) num_bytes != amount )
			break;
	}
	delete[] iov;
	return so_far;
}

static ssize_t sys_writev(int fd, const struct iovec* user_iov, int iovcnt)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	struct iovec* iov = FetchIOV(user_iov, iovcnt);
	if ( !iov )
		return -1;
	ssize_t so_far = 0;
	for ( int i = 0; i < iovcnt && so_far != SSIZE_MAX; i++ )
	{
		const uint8_t* buffer = (const uint8_t*) iov[i].iov_base;
		size_t amount = iov[i].iov_len;
		ssize_t max_left = SSIZE_MAX - so_far;
		if ( (size_t) max_left < amount )
			amount = (size_t) max_left;
		ssize_t num_bytes = desc->write(&ctx, buffer, amount);
		if ( num_bytes < 0 )
		{
			delete[] iov;
			return so_far ? so_far : -1;
		}
		if ( num_bytes == 0 )
			break;
		so_far += num_bytes;

		// TODO: Is this the correct behavior?
		if ( (size_t) num_bytes != amount )
			break;
	}
	delete[] iov;
	return so_far;
}

static ssize_t sys_pwritev(int fd, const struct iovec* user_iov, int iovcnt,
                          off_t offset)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	struct iovec* iov = FetchIOV(user_iov, iovcnt);
	if ( !iov )
		return -1;
	ssize_t so_far = 0;
	for ( int i = 0; i < iovcnt && so_far != SSIZE_MAX; i++ )
	{
		const uint8_t* buffer = (const uint8_t*) iov[i].iov_base;
		size_t amount = iov[i].iov_len;
		ssize_t max_left = SSIZE_MAX - so_far;
		if ( (size_t) max_left < amount )
			amount = (size_t) max_left;
		ssize_t num_bytes = desc->pwrite(&ctx, buffer, amount, offset + so_far);
		if ( num_bytes < 0 )
		{
			delete[] iov;
			return so_far ? so_far : -1;
		}
		if ( num_bytes == 0 )
			break;
		so_far += num_bytes;

		// TODO: Is this the correct behavior?
		if ( (size_t) num_bytes != amount )
			break;
	}
	delete[] iov;
	return so_far;
}

static int sys_mkpartition(int fd, off_t start, off_t length, int flags)
{
	int fdflags = 0;
	if ( flags & O_CLOEXEC ) fdflags |= FD_CLOEXEC;
	if ( flags & O_CLOFORK ) fdflags |= FD_CLOFORK;

	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;

	int dflags = desc->dflags;
	Ref<Inode> inner_inode = desc->vnode->inode;
	desc.Reset();

	Ref<Inode> partition(new Partition(inner_inode, start, length));
	if ( !partition )
		return -1;
	inner_inode.Reset();

	Ref<Vnode> partition_vnode(new Vnode(partition, Ref<Vnode>(NULL), 0, 0));
	if ( !partition_vnode )
		return -1;
	partition.Reset();

	Ref<Descriptor> partition_desc(new Descriptor(partition_vnode, dflags));
	if ( !partition_desc )
		return -1;
	partition_vnode.Reset();

	return CurrentProcess()->GetDTable()->Allocate(partition_desc, fdflags);
}

void Init()
{
	Syscall::Register(SYSCALL_ACCEPT4, (void*) sys_accept4);
	Syscall::Register(SYSCALL_ACCESS, (void*) sys_access);
	Syscall::Register(SYSCALL_BIND, (void*) sys_bind);
	Syscall::Register(SYSCALL_CHDIR, (void*) sys_chdir);
	Syscall::Register(SYSCALL_CHMOD, (void*) sys_chmod);
	Syscall::Register(SYSCALL_CHOWN, (void*) sys_chown);
	Syscall::Register(SYSCALL_CLOSE, (void*) sys_close);
	Syscall::Register(SYSCALL_CONNECT, (void*) sys_connect);
	Syscall::Register(SYSCALL_DUP2, (void*) sys_dup2);
	Syscall::Register(SYSCALL_DUP3, (void*) sys_dup3);
	Syscall::Register(SYSCALL_DUP, (void*) sys_dup);
	Syscall::Register(SYSCALL_FACCESSAT, (void*) sys_faccessat);
	Syscall::Register(SYSCALL_FCHDIRAT, (void*) sys_fchdirat);
	Syscall::Register(SYSCALL_FCHDIR, (void*) sys_fchdir);
	Syscall::Register(SYSCALL_FCHMODAT, (void*) sys_fchmodat);
	Syscall::Register(SYSCALL_FCHMOD, (void*) sys_fchmod);
	Syscall::Register(SYSCALL_FCHOWNAT, (void*) sys_fchownat);
	Syscall::Register(SYSCALL_FCHOWN, (void*) sys_fchown);
	Syscall::Register(SYSCALL_FCHROOTAT, (void*) sys_fchrootat);
	Syscall::Register(SYSCALL_FCHROOT, (void*) sys_fchroot);
	Syscall::Register(SYSCALL_FCNTL, (void*) sys_fcntl);
	Syscall::Register(SYSCALL_FSTATAT, (void*) sys_fstatat);
	Syscall::Register(SYSCALL_FSTAT, (void*) sys_fstat);
	Syscall::Register(SYSCALL_FSYNC, (void*) sys_fsync);
	Syscall::Register(SYSCALL_FTRUNCATE, (void*) sys_ftruncate);
	Syscall::Register(SYSCALL_FUTIMENS, (void*) sys_futimens);
	Syscall::Register(SYSCALL_GETTERMMODE, (void*) sys_gettermmode);
	Syscall::Register(SYSCALL_IOCTL, (void*) sys_ioctl);
	Syscall::Register(SYSCALL_ISATTY, (void*) sys_isatty);
	Syscall::Register(SYSCALL_LINKAT, (void*) sys_linkat);
	Syscall::Register(SYSCALL_LINK, (void*) sys_link);
	Syscall::Register(SYSCALL_LISTEN, (void*) sys_listen);
	Syscall::Register(SYSCALL_MKDIRAT, (void*) sys_mkdirat);
	Syscall::Register(SYSCALL_MKDIR, (void*) sys_mkdir);
	Syscall::Register(SYSCALL_MKPARTITION, (void*) sys_mkpartition);
	Syscall::Register(SYSCALL_OPENAT, (void*) sys_openat);
	Syscall::Register(SYSCALL_OPEN, (void*) sys_open);
	Syscall::Register(SYSCALL_PREAD, (void*) sys_pread);
	Syscall::Register(SYSCALL_PREADV, (void*) sys_preadv);
	Syscall::Register(SYSCALL_PWRITE, (void*) sys_pwrite);
	Syscall::Register(SYSCALL_PWRITEV, (void*) sys_pwritev);
	Syscall::Register(SYSCALL_READDIRENTS, (void*) sys_readdirents);
	Syscall::Register(SYSCALL_READLINKAT, (void*) sys_readlinkat);
	Syscall::Register(SYSCALL_READ, (void*) sys_read);
	Syscall::Register(SYSCALL_READV, (void*) sys_readv);
	Syscall::Register(SYSCALL_RECV, (void*) sys_recv);
	Syscall::Register(SYSCALL_RENAMEAT, (void*) sys_renameat);
	Syscall::Register(SYSCALL_RMDIR, (void*) sys_rmdir);
	Syscall::Register(SYSCALL_SEEK, (void*) sys_seek);
	Syscall::Register(SYSCALL_SEND, (void*) sys_send);
	Syscall::Register(SYSCALL_SETTERMMODE, (void*) sys_settermmode);
	Syscall::Register(SYSCALL_STAT, (void*) sys_stat);
	Syscall::Register(SYSCALL_SYMLINKAT, (void*) sys_symlinkat);
	Syscall::Register(SYSCALL_TCGETPGRP, (void*) sys_tcgetpgrp);
	Syscall::Register(SYSCALL_TCGETWINSIZE, (void*) sys_tcgetwinsize);
	Syscall::Register(SYSCALL_TCSETPGRP, (void*) sys_tcsetpgrp);
	Syscall::Register(SYSCALL_TRUNCATEAT, (void*) sys_truncateat);
	Syscall::Register(SYSCALL_TRUNCATE, (void*) sys_truncate);
	Syscall::Register(SYSCALL_UNLINKAT, (void*) sys_unlinkat);
	Syscall::Register(SYSCALL_UNLINK, (void*) sys_unlink);
	Syscall::Register(SYSCALL_UTIMENSAT, (void*) sys_utimensat);
	Syscall::Register(SYSCALL_WRITE, (void*) sys_write);
	Syscall::Register(SYSCALL_WRITEV, (void*) sys_writev);
}

} // namespace IO
} // namespace Sortix