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

	fs/ramfs.h
	A filesystem stored entirely in RAM.

******************************************************************************/

#include "../platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "../filesystem.h"
#include "../stream.h"
#include "ramfs.h"

using namespace Maxsi;

namespace Sortix
{
	class DevRAMFSFile : public DevBuffer
	{
	public:
		typedef DevBuffer BaseClass;

	public:
		// Transfers ownership of name.
		DevRAMFSFile(char* name);
		virtual ~DevRAMFSFile();

	public:
		char* name;

	private:
		size_t offset;
		byte* buffer;
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
	
	DevRAMFSFile::DevRAMFSFile(char* name)
	{
		this->name = name;
		buffer = NULL;
		buffersize = 0;
	}

	DevRAMFSFile::~DevRAMFSFile()
	{
		delete[] name;
		delete[] buffer;
	}

	size_t DevRAMFSFile::BlockSize()
	{
		return 1;
	}

	uintmax_t DevRAMFSFile::Size()
	{
		return buffersize;
	}

	uintmax_t DevRAMFSFile::Position()
	{
		return offset;
	}

	bool DevRAMFSFile::Seek(uintmax_t position)
	{
		if ( SIZE_MAX < position ) { Error::Set(Error::EOVERFLOW); return false; }
		offset = position;
		return true;
	}

	bool DevRAMFSFile::Resize(uintmax_t size)
	{
		if ( SIZE_MAX < size ) { Error::Set(Error::EOVERFLOW); return false; }
		byte* newbuffer = new byte[size];
		if ( !newbuffer ) { Error::Set(Error::ENOSPC); return false; }
		size_t sharedmemsize = ( size < buffersize ) ? size : buffersize;
		Memory::Copy(newbuffer, buffer, sharedmemsize);
		delete[] buffer;
		buffer = newbuffer;
		buffersize = size;
		return true;
	}

	ssize_t DevRAMFSFile::Read(byte* dest, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		size_t available = count;
		if ( buffersize < offset + count ) { available = buffersize - offset; }
		if ( available == 0 ) { return 0; }
		Memory::Copy(dest, buffer + offset, available);
		offset += available;
		return available;
	}

	ssize_t DevRAMFSFile::Write(const byte* src, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		if ( buffersize < offset + count )
		{
			uintmax_t newsize = (uintmax_t) offset + (uintmax_t) count;
			if ( !Resize(newsize) ) { return -1; }
		}

		Memory::Copy(buffer + offset, src, count);
		offset += count;
		return count;
	}

	bool DevRAMFSFile::IsReadable()
	{
		return true;
	}

	bool DevRAMFSFile::IsWritable()
	{
		return true;
	}

	DevRAMFS::DevRAMFS()
	{
		files = NULL;
	}

	DevRAMFS::~DevRAMFS()
	{
		if ( files )
		{
			while ( !files->Empty() ) { delete files->Remove(0); }
			delete files;
		}
	}

	int CompareFiles(DevRAMFSFile* file1, DevRAMFSFile* file2)
	{
		return String::Compare(file1->name, file2->name);
	}

	int LookupFile(DevRAMFSFile* file, const char* name)
	{
		return String::Compare(file->name, name);
	}

	Device* DevRAMFS::Open(const char* path, int flags, mode_t mode)
	{
		DevBuffer* file = OpenFile(path, flags, mode);
		if ( !file ) { return NULL; }
		Device* wrapper = new DevFileWrapper(file, flags);
		if ( !wrapper ) { Error::Set(Error::ENOSPC); return NULL; }
		return wrapper;
	}

	DevBuffer* DevRAMFS::OpenFile(const char* path, int flags, mode_t mode)
	{
		if ( *path++ != '/' ) { return NULL; }

		if ( files )
		{
			size_t fileindex = files->Search(LookupFile, path);
			if ( fileindex != SIZE_MAX ) { return files->Get(fileindex); }
		}

		return CreateFile(path, flags, mode);
	}

	DevBuffer* DevRAMFS::CreateFile(const char* path, int flags, mode_t mode)
	{
		if ( !(flags & O_CREAT) ) { Error::Set(Error::ENOENT); return NULL; }

		if ( !files )
		{
			files = new SortedList<DevRAMFSFile*>(CompareFiles);
			if ( !files) { Error::Set(Error::ENOSPC); return NULL; }
		}

		if ( files->Search(LookupFile, path) != SIZE_MAX )
		{
			Error::Set(Error::EEXIST);
			return NULL;
		}

		char* newpath = String::Clone(path);
		if ( !newpath ) { Error::Set(Error::ENOSPC); return NULL; }

		DevRAMFSFile* file = new DevRAMFSFile(newpath);
		if ( !file ) { delete[] newpath; Error::Set(Error::ENOSPC); return NULL; }
		if ( !files->Add(file) ) { delete file; Error::Set(Error::ENOSPC); return NULL; }

		file->Refer();

		return file;
	}
}
