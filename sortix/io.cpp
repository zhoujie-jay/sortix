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

	io.cpp
	Provides system calls for input and output.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include "thread.h"
#include "process.h"
#include "device.h"
#include "stream.h"
#include "syscall.h"
#include "io.h"

using namespace Maxsi;

namespace Sortix
{
	namespace IO
	{
		struct SysWrite_t
		{
			union { size_t align1; int fd; };
			union { size_t align2; const byte* buffer; };
			union { size_t align3; size_t count; };
		};

		STATIC_ASSERT(sizeof(SysWrite_t) <= sizeof(Thread::scstate));

		ssize_t SysWrite(int fd, const byte* buffer, size_t count)
		{
			// TODO: Check that buffer is a valid user-space buffer.
			if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { return -1; /* TODO: EBADF */ }
			if ( !dev->IsType(Device::STREAM) ) { return -1; /* TODO: EBADF */ }
			DevStream* stream = (DevStream*) dev;
			if ( !stream->IsWritable() ) { return -1; /* TODO: EBADF */ }
			ssize_t written = stream->Write(buffer, count);
			if ( 0 <= written ) { return written; }
			if ( Error::Last() != Error::EWOULDBLOCK ) { return -1; /* TODO: errno */ }

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
		}

		struct SysRead_t
		{
			union { size_t align1; int fd; };
			union { size_t align2; byte* buffer; };
			union { size_t align3; size_t count; };
		};

		STATIC_ASSERT(sizeof(SysRead_t) <= sizeof(Thread::scstate));

		ssize_t SysRead(int fd, byte* buffer, size_t count)
		{
			// TODO: Check that buffer is a valid user-space buffer.
			if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { return -1; /* TODO: EBADF */ }
			if ( !dev->IsType(Device::STREAM) ) { return -1; /* TODO: EBADF */ }
			DevStream* stream = (DevStream*) dev;
			if ( !stream->IsReadable() ) { return -1; /* TODO: EBADF */ }
			ssize_t bytesread = stream->Read(buffer, count);
			if ( 0 <= bytesread ) { return bytesread; }
			if ( Error::Last() != Error::EWOULDBLOCK ) { return -1; /* TODO: errno */ }

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
		}

		int SysClose(int fd)
		{
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { return -1; /* TODO: EBADF */ }
			process->descriptors.Free(fd);
			return 0;
		}

		int SysDup(int fd)
		{
			Process* process = CurrentProcess();
			Device* dev = process->descriptors.Get(fd);
			if ( !dev ) { return -1; /* TODO: EBADF */ }
			process->descriptors.Free(fd);
			return process->descriptors.Allocate(dev);
		}

		void Init()
		{
			Syscall::Register(SYSCALL_WRITE, (void*) SysWrite);
			Syscall::Register(SYSCALL_READ, (void*) SysRead);
			Syscall::Register(SYSCALL_CLOSE, (void*) SysClose);
			Syscall::Register(SYSCALL_DUP, (void*) SysDup);
		}
	}
}
