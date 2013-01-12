/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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
#include <sortix/kernel/string.h>
#include <sortix/kernel/kthread.h>
#include <sortix/seek.h>
#include <errno.h>
#include "thread.h"
#include "process.h"
#include "device.h"
#include "stream.h"
#include "syscall.h"
#include "io.h"

namespace Sortix
{
	namespace IO
	{
#ifdef GOT_FAKE_KTHREAD
		struct SysWrite_t
		{
			union { size_t align1; int fd; };
			union { size_t align2; const uint8_t* buffer; };
			union { size_t align3; size_t count; };
		};

		STATIC_ASSERT(sizeof(SysWrite_t) <= sizeof(Thread::scstate));
#endif

		ssize_t SysWrite(int fd, const uint8_t* buffer, size_t count)
		{
			// TODO: Check that buffer is a valid user-space buffer.
			if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			if ( !dev->IsType(Device::STREAM) ) { errno = EBADF; return -1; }
			DevStream* stream = (DevStream*) dev;
			if ( !stream->IsWritable() ) { errno = EBADF; return -1; }
#ifdef GOT_FAKE_KTHREAD
			ssize_t written = stream->Write(buffer, count);
			if ( 0 <= written ) { return written; }
			if ( errno != EBLOCKING ) { return -1; }

			// The stream will resume our system call once progress has been
			// made. Our request is certainly not forgotten.

			// Resume the system call with these parameters.
			Thread* thread = CurrentThread();
			thread->scfunc = (void*) SysWrite;
			SysWrite_t* state = (SysWrite_t*) thread->scstate;
			state->fd = fd;
			state->buffer = buffer;
			state->count = count;
			thread->scsize = sizeof(SysWrite_t);

			// Now go do something else.
			Syscall::Incomplete();
			return 0;
#else
			return stream->Write(buffer, count);
#endif
		}

		// TODO: Not implemented yet due to stupid internal kernel design.
		ssize_t SysPWrite(int fd, const uint8_t* buffer, size_t count, off_t off)
		{
			errno = ENOSYS;
			return -1;
		}

#ifdef GOT_FAKE_KTHREAD
		struct SysRead_t
		{
			union { size_t align1; int fd; };
			union { size_t align2; uint8_t* buffer; };
			union { size_t align3; size_t count; };
		};

		STATIC_ASSERT(sizeof(SysRead_t) <= sizeof(Thread::scstate));
#endif

		ssize_t SysRead(int fd, uint8_t* buffer, size_t count)
		{
			// TODO: Check that buffer is a valid user-space buffer.
			if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			if ( !dev->IsType(Device::STREAM) ) { errno = EBADF; return -1; }
			DevStream* stream = (DevStream*) dev;
			if ( !stream->IsReadable() ) { errno = EBADF; return -1;}
#ifdef GOT_FAKE_KTHREAD
			ssize_t bytesread = stream->Read(buffer, count);
			if ( 0 <= bytesread ) { return bytesread; }
			if ( errno != EBLOCKING ) { return -1; }

			// The stream will resume our system call once progress has been
			// made. Our request is certainly not forgotten.

			// Resume the system call with these parameters.
			Thread* thread = CurrentThread();
			thread->scfunc = (void*) SysRead;
			SysRead_t* state = (SysRead_t*) thread->scstate;
			state->fd = fd;
			state->buffer = buffer;
			state->count = count;
			thread->scsize = sizeof(SysRead_t);

			// Now go do something else.
			Syscall::Incomplete();
			return 0;
#else
			return stream->Read(buffer, count);
#endif
		}

		// TODO: Not implemented yet due to stupid internal kernel design.
		ssize_t SysPRead(int fd, uint8_t* buffer, size_t count, off_t off)
		{
			errno = ENOSYS;
			return -1;
		}

		int SysSeek(int fd, off_t* offset, int whence)
		{
			// TODO: Validate that offset is a legal user-space off_t!
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { errno = EBADF; *offset = -1; return -1; }
			if ( !dev->IsType(Device::BUFFER) ) { errno = EBADF; *offset = -1; return -1; }
			DevBuffer* buffer = (DevBuffer*) dev;
			off_t origin;
			switch ( whence )
			{
				case SEEK_SET: origin = 0; break;
				case SEEK_CUR: origin = buffer->Position(); break;
				case SEEK_END: origin = buffer->Size(); break;
				default: errno = EINVAL; *offset = -1; return -1;
			}
			off_t newposition = origin + *offset;
			if ( newposition < 0 ) { errno = EINVAL; *offset = -1; return -1; }
			if ( !buffer->Seek(newposition) ) { *offset = -1; return -1; }
			*offset = buffer->Position();
			return 0;
		}

		int SysClose(int fd)
		{
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			process->descriptors.Free(fd);
			return 0;
		}

		int SysDup(int fd)
		{
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { errno = EBADF; return -1; }
			const char* path = process->descriptors.GetPath(fd);
			char* pathcopy = path ? String::Clone(path) : NULL;
			return process->descriptors.Allocate(dev, pathcopy);
		}

		void Init()
		{
			Syscall::Register(SYSCALL_WRITE, (void*) SysWrite);
			Syscall::Register(SYSCALL_PWRITE, (void*) SysPWrite);
			Syscall::Register(SYSCALL_READ, (void*) SysRead);
			Syscall::Register(SYSCALL_PREAD, (void*) SysPRead);
			Syscall::Register(SYSCALL_CLOSE, (void*) SysClose);
			Syscall::Register(SYSCALL_DUP, (void*) SysDup);
			Syscall::Register(SYSCALL_SEEK, (void*) SysSeek);
		}
	}
}
