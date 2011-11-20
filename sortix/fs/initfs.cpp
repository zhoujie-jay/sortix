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

	fs/initfs.h
	Provides access to the initial ramdisk.

******************************************************************************/

#include "../platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "../filesystem.h"
#include "../stream.h"
#include "initfs.h"
#include "../initrd.h"

using namespace Maxsi;

namespace Sortix
{
	class DevInitFSFile : public DevBuffer
	{
	public:
		typedef DevBuffer BaseClass;

	public:
		// Transfers ownership of name.
		DevInitFSFile(char* name, const byte* buffer, size_t buffersize);
		virtual ~DevInitFSFile();

	public:
		char* name;

	private:
		size_t offset;
		const byte* buffer;
		size_t buffersize;

	public:
		virtual size_t BlockSize();
		virtual uintmax_t Size();
		virtual uintmax_t Position();
		virtual bool Seek(uintmax_t position);
		virtual bool Resize(uintmax_t size);
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};
	
	DevInitFSFile::DevInitFSFile(char* name, const byte* buffer, size_t buffersize)
	{
		this->name = name;
		this->buffer = buffer;
		this->buffersize = buffersize;
	}

	DevInitFSFile::~DevInitFSFile()
	{
		delete[] name;
	}

	size_t DevInitFSFile::BlockSize()
	{
		return 1;
	}

	uintmax_t DevInitFSFile::Size()
	{
		return buffersize;
	}

	uintmax_t DevInitFSFile::Position()
	{
		return offset;
	}

	bool DevInitFSFile::Seek(uintmax_t position)
	{
		if ( SIZE_MAX < position ) { Error::Set(Error::EOVERFLOW); return false; }
		offset = position;
		return true;
	}

	bool DevInitFSFile::Resize(uintmax_t /*size*/)
	{
		Error::Set(Error::EBADF);
		return false;
	}

	ssize_t DevInitFSFile::Read(byte* dest, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		size_t available = count;
		if ( buffersize < offset + count ) { available = buffersize - offset; }
		if ( available == 0 ) { return 0; }
		Memory::Copy(dest, buffer + offset, available);
		offset += available;
		return available;
	}

	ssize_t DevInitFSFile::Write(const byte* /*src*/, size_t /*count*/)
	{
		Error::Set(Error::EBADF);
		return false;
	}

	bool DevInitFSFile::IsReadable()
	{
		return true;
	}

	bool DevInitFSFile::IsWritable()
	{
		return false;
	}

	DevInitFS::DevInitFS()
	{
	}

	DevInitFS::~DevInitFS()
	{
	}

	Device* DevInitFS::Open(const char* path, int flags, mode_t mode)
	{
		if ( *path++ != '/' ) { return NULL; }

		if ( (flags & O_LOWERFLAGS) != O_RDONLY ) { Error::Set(Error::EROFS); return NULL; }

		size_t buffersize;
		const byte* buffer = InitRD::Open(path, &buffersize);
		if ( !buffer ) { Error::Set(Error::ENOENT); return NULL; }

		char* newpath = String::Clone(path);
		if ( !newpath ) { Error::Set(Error::ENOSPC); return NULL; }

		Device* result = new DevInitFSFile(newpath, buffer, buffersize);
		if ( !result ) { delete[] newpath; Error::Set(Error::ENOSPC); return NULL; }

		return result;
	}
}

