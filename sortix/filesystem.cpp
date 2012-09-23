/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	filesystem.cpp
	Allows access to stored sequences of bytes in an orderly fashion.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <errno.h>
#include <string.h>
#include "syscall.h"
#include "process.h"
#include "filesystem.h"
#include "directory.h"
#include "mount.h"
#include <sortix/stat.h>
#include <sortix/fcntl.h>
#include <sortix/unistd.h>

namespace Sortix
{
	namespace FileSystem
	{
		Device* Open(const char* path, int flags, mode_t mode)
		{
			Process* process = CurrentProcess();
			const char* wd = process->workingdir;
			char* abs = Directory::MakeAbsolute(wd, path);
			if ( !abs ) { errno = ENOMEM; return NULL; }

			size_t pathoffset = 0;
			DevFileSystem* fs = Mount::WhichFileSystem(abs, &pathoffset);
			if ( !fs ) { delete[] abs; return NULL; }
			Device* result = fs->Open(abs + pathoffset, flags, mode);
			delete[] abs;
			return result;
		}

		bool Unlink(const char* path)
		{
			Process* process = CurrentProcess();
			const char* wd = process->workingdir;
			char* abs = Directory::MakeAbsolute(wd, path);
			if ( !abs ) { errno = ENOMEM; return false; }

			size_t pathoffset = 0;
			DevFileSystem* fs = Mount::WhichFileSystem(abs, &pathoffset);
			if ( !fs ) { delete[] abs; return false; }
			bool result = fs->Unlink(abs + pathoffset);
			delete[] abs;
			return result;
		}

		int SysOpen(const char* path, int flags, mode_t mode)
		{
			Process* process = CurrentProcess();
			Device* dev = Open(path, flags, mode);
			if ( !dev ) { return -1; /* TODO: errno */ }
			int fd = process->descriptors.Allocate(dev);
			if ( fd < 0 ) { dev->Unref(); }
			int fdflags = 0;
			if ( flags & O_CLOEXEC ) { fdflags |= FD_CLOEXEC; }
			if ( flags & O_CLOFORK ) { fdflags |= FD_CLOFORK; }
			process->descriptors.SetFlags(fd, fdflags);
			return fd;
		}

		int SysAccess(const char* pathname, int mode)
		{
			int oflags = 0;
			bool exec = mode & X_OK;
			bool read = mode & R_OK;
			bool write = mode & W_OK;
			if ( mode == F_OK ) { oflags = O_RDONLY; }
			if ( exec && !read && !write ) { oflags = O_EXEC; }
			if ( exec && read && !write ) { oflags = O_EXEC; }
			if ( exec && !read && write ) { oflags = O_EXEC; }
			if ( exec && read && write ) { oflags = O_EXEC; }
			if ( !exec && read && write ) { oflags = O_RDWR; }
			if ( !exec && !read && write ) { oflags = O_WRONLY; }
			if ( !exec && read && !write ) { oflags = O_RDONLY; }
			Device* dev = Open(pathname, oflags, 0);
			if ( !dev ) { return -1; }
			dev->Unref();
			return 0;
		}

		int SysUnlink(const char* path)
		{
			return Unlink(path) ? 0 : -1;
		}

		int SysMkDir(const char* pathname, mode_t mode)
		{
			// TODO: Add the proper filesystem support!
			errno = ENOSYS;
			return -1;
		}

		int SysRmDir(const char* pathname)
		{
			// TODO: Add the proper filesystem support!
			errno = ENOSYS;
			return -1;
		}

		int SysTruncate(const char* pathname, off_t length)
		{
			// TODO: Add the proper filesystem support!
			errno = ENOSYS;
			return -1;
		}

		int SysFTruncate(const char* pathname, off_t length)
		{
			// TODO: Add the proper filesystem support!
			errno = ENOSYS;
			return -1;
		}

		void HackStat(Device* dev, struct stat* st)
		{
			memset(st, 0, sizeof(*st));
			st->st_mode = 0777;
			st->st_nlink = 1;
			if ( dev->IsType(Device::BUFFER) )
			{
				st->st_mode |= S_IFREG;
				DevBuffer* buffer = (DevBuffer*) dev;
				st->st_size = buffer->Size();
				st->st_blksize = 1;
				st->st_blocks = st->st_size;
			}
			if ( dev->IsType(Device::DIRECTORY) )
			{
				st->st_mode |= S_IFDIR;
				st->st_nlink = 2;
			}
		}

		int SysStat(const char* pathname, struct stat* st)
		{
			Device* dev = Open(pathname, O_RDONLY, 0);
			if ( !dev && errno == EISDIR )
			{
				dev = Open(pathname, O_SEARCH, 0);
			}
			if ( !dev ) { return -1; }
			HackStat(dev, st);
			dev->Unref();
			return 0;
		}

		int SysFStat(int fd, struct stat* st)
		{
			Process* process = CurrentProcess();
			DescriptorTable* descs = &(process->descriptors);
			Device* dev = descs->Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			HackStat(dev, st);
			return 0;
		}

		int SysFCntl(int fd, int cmd, unsigned long arg)
		{
			Process* process = CurrentProcess();
			DescriptorTable* descs = &(process->descriptors);
			Device* dev = descs->Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			switch ( cmd )
			{
			case F_SETFD: descs->SetFlags(fd, (int) arg); return 0;
			case F_GETFD: return descs->GetFlags(fd);
			case F_SETFL: errno = ENOSYS; return -1;
			case F_GETFL:
					if ( dev->IsType(Device::DIRECTORY) ) { return O_SEARCH; }
					if ( !dev->IsType(Device::STREAM) )
					{
						errno = ENOSYS;
						return -1;
					}
					DevStream* stream = (DevStream*) stream;
					bool write = stream->IsWritable();
					bool read = stream->IsReadable();
					if ( write && read ) { return O_RDWR; }
					if ( write && !read ) { return O_WRONLY; }
					if ( !write && read ) { return O_RDONLY; }
					return O_EXEC;
				break;
			}
			errno = EINVAL;
			return 1;
		}

		void Init()
		{
			Syscall::Register(SYSCALL_OPEN, (void*) SysOpen);
			Syscall::Register(SYSCALL_UNLINK, (void*) SysUnlink);
			Syscall::Register(SYSCALL_MKDIR, (void*) SysMkDir);
			Syscall::Register(SYSCALL_RMDIR, (void*) SysRmDir);
			Syscall::Register(SYSCALL_TRUNCATE, (void*) SysTruncate);
			Syscall::Register(SYSCALL_FTRUNCATE, (void*) SysFTruncate);
			Syscall::Register(SYSCALL_STAT, (void*) SysStat);
			Syscall::Register(SYSCALL_FSTAT, (void*) SysFStat);
			Syscall::Register(SYSCALL_FCNTL, (void*) SysFCntl);
			Syscall::Register(SYSCALL_ACCESS, (void*) SysAccess);
		}
	}

	DevFileWrapper::DevFileWrapper(DevBuffer* buffer, int flags)
	{
		this->buffer = buffer;
		this->flags = flags;
		this->offset = 0;
		this->buffer->Refer();
		if ( flags & O_TRUNC ) { buffer->Resize(0); }
	}

	DevFileWrapper::~DevFileWrapper()
	{
		buffer->Unref();
	}

	size_t DevFileWrapper::BlockSize()
	{
		return buffer->BlockSize();
	}

	uintmax_t DevFileWrapper::Size()
	{
		return buffer->Size();
	}

	uintmax_t DevFileWrapper::Position()
	{
		return offset;
	}

	bool DevFileWrapper::Seek(uintmax_t position)
	{
		if ( !buffer->Seek(position) ) { return false; }
		offset = position;
		return true;
	}

	bool DevFileWrapper::Resize(uintmax_t size)
	{
		return buffer->Resize(size);
	}

	ssize_t DevFileWrapper::Read(uint8_t* dest, size_t count)
	{
		// TODO: Enforce read permission!
		if ( !buffer->Seek(offset) ) { return -1; }
		ssize_t result = buffer->Read(dest, count);
		if ( result < 0 ) { return result; }
		offset += result;
		return result;
	}

	ssize_t DevFileWrapper::Write(const uint8_t* src, size_t count)
	{
		// TODO: Enforce write permission!
		if ( !buffer->Seek(offset) ) { return -1; }
		ssize_t result = buffer->Write(src, count);
		if ( result < 0 ) { return result; }
		offset += result;
		return result;
	}

	bool DevFileWrapper::IsReadable()
	{
		// TODO: Enforce read permission!
		return true;
	}

	bool DevFileWrapper::IsWritable()
	{
		// TODO: Enforce write permission!
		return true;
	}
}


