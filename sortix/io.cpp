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

#include <sortix/kernel/platform.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/kthread.h>

#include <sortix/seek.h>
#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/stat.h>
#include <assert.h>
#include <errno.h>
#include "thread.h"
#include "process.h"
#include "syscall.h"
#include "io.h"

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

static int sys_dup2(int oldfd, int newfd)
{
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	return dtable->Copy(oldfd, newfd);
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

static int sys_open(const char* path, int flags, mode_t mode)
{
	return sys_openat(AT_FDCWD, path, flags, mode);
}

// TODO: This is a hack! Stat the file in some manner and check permissions.
static int sys_access(const char* path, int /*mode*/)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_RDONLY);
	delete[] pathcopy;
	return desc ? 0 : -1;
}

static int sys_unlink(const char* path)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	int ret = from->unlink(&ctx, relpath);
	delete[] pathcopy;
	return ret;
}

// TODO: unlinkat

static int sys_mkdir(const char* path, mode_t mode)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	int ret = from->mkdir(&ctx, relpath, mode);
	delete[] pathcopy;
	return ret;
}

// TODO: mkdirat

static int sys_rmdir(const char* path)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	int ret = from->rmdir(&ctx, relpath);
	delete[] pathcopy;
	return ret;
}

// TODO: unlinkat(AT_REMOVEDIR)

static int sys_truncate(const char* path, off_t length)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_WRONLY);
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->truncate(&ctx, length);
}

static int sys_ftruncate(int fd, off_t length)
{
	Ref<Descriptor> desc = CurrentProcess()->GetDescriptor(fd);
	if ( !desc )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	return desc->truncate(&ctx, length);
}

static int sys_fstatat(int dirfd, const char* path, struct stat* st, int /*flags*/)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath, dirfd);
	if ( !from ) { delete[] pathcopy; return -1; }
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_RDONLY);
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->stat(&ctx, st);
}

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

static int sys_fcntl(int fd, int cmd, unsigned long arg)
{
	Ref<DescriptorTable> dtable = CurrentProcess()->GetDTable();
	int ret = -1;
	switch ( cmd )
	{
	case F_SETFD:
		ret = dtable->SetFlags(fd, (int) arg) ? 0 : -1;
		break;
	case F_GETFD:
		ret = dtable->GetFlags(fd);
		break;
	case F_SETFL:
	case F_GETFL:
		errno = ENOSYS;
		break;
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

static int sys_chdir(const char* path)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_RDONLY | O_DIRECTORY);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	CurrentProcess()->SetCWD(desc);
	return 0;
}

static int sys_chown(const char* path, uid_t owner, gid_t group)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_WRONLY);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->chown(&ctx, owner, group);
}

static int sys_chmod(const char* path, mode_t mode)
{
	char* pathcopy = GetStringFromUser(path);
	if ( !pathcopy )
		return -1;
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	const char* relpath = pathcopy;
	Ref<Descriptor> from = PrepareLookup(&relpath);
	Ref<Descriptor> desc = from->open(&ctx, relpath, O_WRONLY);
	from.Reset();
	delete[] pathcopy;
	if ( !desc )
		return -1;
	return desc->chmod(&ctx, mode);
}

static int sys_link(const char* oldpath, const char* newpath)
{
	ioctx_t ctx; SetupUserIOCtx(&ctx);

	char* newpathcopy = GetStringFromUser(newpath);
	if ( !newpathcopy )
		return -1;
	const char* newrelpath = newpathcopy;
	Ref<Descriptor> newfrom(PrepareLookup(&newrelpath));

	char* final_elem;
	Ref<Descriptor> dir = OpenDirContainingPath(&ctx, newfrom, newpathcopy,
	                                            &final_elem);
	delete[] newpathcopy;
	if ( !dir )
		return -1;

	char* oldpathcopy = GetStringFromUser(oldpath);
	if ( !oldpathcopy ) { delete[] final_elem; return -1; }
	const char* oldrelpath = oldpathcopy;
	Ref<Descriptor> oldfrom(PrepareLookup(&oldrelpath));

	Ref<Descriptor> file = oldfrom->open(&ctx, oldrelpath, O_RDONLY);
	delete[] oldpathcopy;
	if ( !file ) { delete[] final_elem; return -1; }

	int ret = dir->link(&ctx, final_elem, file);
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

void Init()
{
	Syscall::Register(SYSCALL_ACCESS, (void*) sys_access);
	Syscall::Register(SYSCALL_CHDIR, (void*) sys_chdir);
	Syscall::Register(SYSCALL_CHMOD, (void*) sys_chmod);
	Syscall::Register(SYSCALL_CHOWN, (void*) sys_chown);
	Syscall::Register(SYSCALL_CLOSE, (void*) sys_close);
	Syscall::Register(SYSCALL_DUP, (void*) sys_dup);
	Syscall::Register(SYSCALL_DUP2, (void*) sys_dup2);
	Syscall::Register(SYSCALL_FCNTL, (void*) sys_fcntl);
	Syscall::Register(SYSCALL_FSTATAT, (void*) sys_fstatat);
	Syscall::Register(SYSCALL_FSTAT, (void*) sys_fstat);
	Syscall::Register(SYSCALL_FTRUNCATE, (void*) sys_ftruncate);
	Syscall::Register(SYSCALL_GETTERMMODE, (void*) sys_gettermmode);
	Syscall::Register(SYSCALL_ISATTY, (void*) sys_isatty);
	Syscall::Register(SYSCALL_LINK, (void*) sys_link);
	Syscall::Register(SYSCALL_MKDIR, (void*) sys_mkdir);
	Syscall::Register(SYSCALL_OPENAT, (void*) sys_openat);
	Syscall::Register(SYSCALL_OPEN, (void*) sys_open);
	Syscall::Register(SYSCALL_PREAD, (void*) sys_pread);
	Syscall::Register(SYSCALL_PWRITE, (void*) sys_pwrite);
	Syscall::Register(SYSCALL_READDIRENTS, (void*) sys_readdirents);
	Syscall::Register(SYSCALL_READ, (void*) sys_read);
	Syscall::Register(SYSCALL_RMDIR, (void*) sys_rmdir);
	Syscall::Register(SYSCALL_SEEK, (void*) sys_seek);
	Syscall::Register(SYSCALL_SETTERMMODE, (void*) sys_settermmode);
	Syscall::Register(SYSCALL_STAT, (void*) sys_stat);
	Syscall::Register(SYSCALL_TCGETWINSIZE, (void*) sys_tcgetwinsize);
	Syscall::Register(SYSCALL_TRUNCATE, (void*) sys_truncate);
	Syscall::Register(SYSCALL_UNLINK, (void*) sys_unlink);
	Syscall::Register(SYSCALL_WRITE, (void*) sys_write);
}

} // namespace IO
} // namespace Sortix
