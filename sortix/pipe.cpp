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

	pipe.cpp
	A device with a writing end and a reading end.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "event.h"
#include "thread.h"
#include "process.h"
#include "syscall.h"
#include "pipe.h"

using namespace Maxsi;

namespace Sortix
{
	class DevPipeStorage : public DevStream
	{
	public:
		typedef Device BaseClass;

	public:
		DevPipeStorage(byte* buffer, size_t buffersize);
		~DevPipeStorage();

	private:
		byte* buffer;
		size_t buffersize;
		size_t bufferoffset;
		size_t bufferused;
		Event readevent;
		Event writeevent;
		bool anyreading;
		bool anywriting;

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	public:
		void NotReading();
		void NotWriting();

	};

	DevPipeStorage::DevPipeStorage(byte* buffer, size_t buffersize)
	{
		this->buffer = buffer;
		this->buffersize = buffersize;
		this->bufferoffset = 0;
		this->bufferused = 0;
		this->anyreading = true;
		this->anywriting = true;
	}

	DevPipeStorage::~DevPipeStorage()
	{
		delete[] buffer;
	}

	bool DevPipeStorage::IsReadable() { return true; }
	bool DevPipeStorage::IsWritable() { return true; }

	ssize_t DevPipeStorage::Read(byte* dest, size_t count)
	{
		if ( count == 0 ) { return 0; }
		if ( bufferused )
		{
			if ( bufferused < count ) { count = bufferused; }
			size_t amount = count;
			size_t linear = buffersize - bufferoffset;
			if ( linear < amount ) { amount = linear; }
			ASSERT(amount);
			Memory::Copy(dest, buffer + bufferoffset, amount);
			bufferoffset = (bufferoffset + amount) % buffersize;
			bufferused -= amount;
			writeevent.Signal();
			return amount;
		}

		if ( !anywriting ) { return 0; }

		Error::Set(EBLOCKING);
		readevent.Register();
		return -1;
	}

	ssize_t DevPipeStorage::Write(const byte* src, size_t count)
	{
		if ( !anyreading ) { /* TODO: SIGPIPE */ }
		if ( count == 0 ) { return 0; }
		if ( bufferused < buffersize )
		{
			if ( buffersize - bufferused < count ) { count = buffersize - bufferused; }
			size_t writeoffset = (bufferoffset + bufferused) % buffersize;
			size_t amount = count;
			size_t linear = buffersize - writeoffset;
			if ( linear < amount ) { amount = linear; }
			ASSERT(amount);
			Memory::Copy(buffer + writeoffset, src, amount);
			bufferused += amount;
			readevent.Signal();
			return amount;
		}

		Error::Set(EBLOCKING);
		writeevent.Register();
		return -1;
	}

	void DevPipeStorage::NotReading() { anyreading = false; }
	void DevPipeStorage::NotWriting() { anywriting = false; }

	class DevPipeReading : public DevStream
	{
	public:
		typedef Device BaseClass;

	public:
		DevPipeReading(DevStream* stream);
		~DevPipeReading();

	private:
		DevStream* stream;

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};

	DevPipeReading::DevPipeReading(DevStream* stream)
	{
		stream->Refer();
		this->stream = stream;
	}

	DevPipeReading::~DevPipeReading()
	{
		((DevPipeStorage*) stream)->NotReading();
		stream->Unref();
	}

	ssize_t DevPipeReading::Read(byte* dest, size_t count)
	{
		return stream->Read(dest, count);
	}

	ssize_t DevPipeReading::Write(const byte* /*src*/, size_t /*count*/)
	{
		Error::Set(EBADF);
		return -1;
	}

	bool DevPipeReading::IsReadable()
	{
		return true;
	}

	bool DevPipeReading::IsWritable()
	{
		return false;
	}

	class DevPipeWriting : public DevStream
	{
	public:
		typedef Device BaseClass;

	public:
		DevPipeWriting(DevStream* stream);
		~DevPipeWriting();

	private:
		DevStream* stream;

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};

	DevPipeWriting::DevPipeWriting(DevStream* stream)
	{
		stream->Refer();
		this->stream = stream;
	}

	DevPipeWriting::~DevPipeWriting()
	{
		((DevPipeStorage*) stream)->NotWriting();
		stream->Unref();
	}

	ssize_t DevPipeWriting::Read(byte* /*dest*/, size_t /*count*/)
	{
		Error::Set(EBADF);
		return -1;
	}

	ssize_t DevPipeWriting::Write(const byte* src, size_t count)
	{
		return stream->Write(src, count);
	}

	bool DevPipeWriting::IsReadable()
	{
		return false;
	}

	bool DevPipeWriting::IsWritable()
	{
		return true;
	}

	namespace Pipe
	{
		const size_t BUFFER_SIZE = 4096UL;

		int SysPipe(int pipefd[2])
		{
			// TODO: Validate that pipefd is a valid user-space array!
			
			size_t buffersize = BUFFER_SIZE;
			byte* buffer = new byte[buffersize];
			if ( !buffer ) { return -1; /* TODO: ENOMEM */ }

			// Transfer ownership of the buffer to the storage device.
			DevStream* storage = new DevPipeStorage(buffer, buffersize);
			if ( !storage ) { delete[] buffer; return -1; /* TODO: ENOMEM */ }

			DevStream* reading = new DevPipeReading(storage);
			if ( !reading ) { delete storage; return -1; /* TODO: ENOMEM */ }

			DevStream* writing = new DevPipeWriting(storage);
			if ( !writing ) { delete reading; return -1; /* TODO: ENOMEM */ }

			Process* process = CurrentProcess();
			int readfd = process->descriptors.Allocate(reading);
			int writefd = process->descriptors.Allocate(writing);

			if ( readfd < 0 || writefd < 0 )
			{
				if ( 0 <= readfd ) { process->descriptors.Free(readfd); } else { delete reading; }
				if ( 0 <= writefd ) { process->descriptors.Free(writefd); } else { delete writing; }

				return -1; /* TODO: ENOMEM/EMFILE/ENFILE */ 
			}

			pipefd[0] = readfd;
			pipefd[1] = writefd;

			return 0;
		}

		void Init()
		{
			Syscall::Register(SYSCALL_PIPE, (void*) SysPipe);
		}
	}
}
