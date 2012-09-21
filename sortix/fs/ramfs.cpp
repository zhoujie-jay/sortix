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

#include <sortix/kernel/platform.h>
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "../filesystem.h"
#include "../directory.h"
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
		uint8_t* buffer;
		size_t bufferused;
		size_t buffersize;

	public:
		virtual size_t BlockSize();
		virtual uintmax_t Size();
		virtual uintmax_t Position();
		virtual bool Seek(uintmax_t position);
		virtual bool Resize(uintmax_t size);
		virtual ssize_t Read(uint8_t* dest, size_t count);
		virtual ssize_t Write(const uint8_t* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};

	DevRAMFSFile::DevRAMFSFile(char* name)
	{
		this->name = name;
		buffer = NULL;
		bufferused = 0;
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
		return bufferused;
	}

	uintmax_t DevRAMFSFile::Position()
	{
		return offset;
	}

	bool DevRAMFSFile::Seek(uintmax_t position)
	{
		if ( SIZE_MAX < position ) { Error::Set(EOVERFLOW); return false; }
		offset = position;
		return true;
	}

	bool DevRAMFSFile::Resize(uintmax_t size)
	{
		if ( SIZE_MAX < size ) { Error::Set(EOVERFLOW); return false; }
		uint8_t* newbuffer = new uint8_t[size];
		if ( !newbuffer ) { Error::Set(ENOSPC); return false; }
		size_t sharedmemsize = ( size < bufferused ) ? size : bufferused;
		Memory::Copy(newbuffer, buffer, sharedmemsize);
		delete[] buffer;
		buffer = newbuffer;
		bufferused = sharedmemsize;
		buffersize = size;
		return true;
	}

	ssize_t DevRAMFSFile::Read(uint8_t* dest, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		size_t available = count;
		if ( bufferused < offset + count ) { available = bufferused - offset; }
		if ( available == 0 ) { return 0; }
		Memory::Copy(dest, buffer + offset, available);
		offset += available;
		return available;
	}

	ssize_t DevRAMFSFile::Write(const uint8_t* src, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		if ( buffersize < offset + count )
		{
			uintmax_t newsize = (uintmax_t) offset + (uintmax_t) count;
			if ( newsize < buffersize * 2 ) { newsize = buffersize * 2; }
			if ( !Resize(newsize) ) { return -1; }
		}

		Memory::Copy(buffer + offset, src, count);
		offset += count;
		if ( bufferused < offset ) { bufferused = offset; }
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

	class DevRAMFSDir : public DevDirectory
	{
	public:
		typedef Device DevDirectory;

	public:
		DevRAMFSDir(DevRAMFS* fs);
		virtual ~DevRAMFSDir();

	private:
		DevRAMFS* fs;
		size_t position;

	public:
		virtual void Rewind();
		virtual int Read(sortix_dirent* dirent, size_t available);

	};

	DevRAMFSDir::DevRAMFSDir(DevRAMFS* fs)
	{
		position = 0;
		this->fs = fs;
		fs->Refer();
	}

	DevRAMFSDir::~DevRAMFSDir()
	{
		fs->Unref();
	}

	void DevRAMFSDir::Rewind()
	{
		position = 0;
	}

	int DevRAMFSDir::Read(sortix_dirent* dirent, size_t available)
	{
		if ( available <= sizeof(sortix_dirent) ) { return -1; }
		if ( fs->GetNumFiles() <= position )
		{
			dirent->d_namelen = 0;
			dirent->d_name[0] = 0;
			return 0;
		}

		const char* name = fs->GetFilename(position);
		if ( !name ) { return -1; }
		size_t namelen = String::Length(name);
		size_t needed = sizeof(sortix_dirent) + namelen + 1;

		if ( available < needed )
		{
			dirent->d_namelen = needed;
			Error::Set(ERANGE);
			return -1;
		}

		Memory::Copy(dirent->d_name, name, namelen + 1);
		dirent->d_namelen = namelen;
		position++;
		return 0;
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
		if ( path[0] == 0 || (path[0] == '/' && path[1] == 0) )
		{
			if ( (flags & O_LOWERFLAGS) == O_SEARCH )
			{
				return new DevRAMFSDir(this);
			}

			Error::Set(EISDIR);
			return NULL;
		}

		if ( (flags & O_LOWERFLAGS) == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }

		if ( *path++ != '/' ) { Error::Set(ENOENT); return NULL; }

		size_t pathlen = String::Length(path);
		for ( size_t i = 0; i < pathlen; i++ )
		{
			if ( path[i] == '/' ) { Error::Set(ENOENT); return NULL; }
		}

		DevBuffer* file = OpenFile(path, flags, mode);
		if ( !file ) { return NULL; }
		Device* wrapper = new DevFileWrapper(file, flags);
		if ( !wrapper ) { Error::Set(ENOSPC); return NULL; }
		return wrapper;
	}

	DevBuffer* DevRAMFS::OpenFile(const char* path, int flags, mode_t mode)
	{
		// Hack to prevent / from being a filename.
		if ( path == 0 ) { Error::Set(ENOENT); return NULL; }

		if ( files )
		{
			size_t fileindex = files->Search(LookupFile, path);
			if ( fileindex != SIZE_MAX )
			{
				DevRAMFSFile* file = files->Get(fileindex);
				if ( flags & O_TRUNC ) { file->Resize(0); }
				return file;
			}
		}

		return CreateFile(path, flags, mode);
	}

	DevBuffer* DevRAMFS::CreateFile(const char* path, int flags, mode_t mode)
	{
		if ( !(flags & O_CREAT) ) { Error::Set(ENOENT); return NULL; }

		if ( !files )
		{
			files = new SortedList<DevRAMFSFile*>(CompareFiles);
			if ( !files) { Error::Set(ENOSPC); return NULL; }
		}

		if ( files->Search(LookupFile, path) != SIZE_MAX )
		{
			Error::Set(EEXIST);
			return NULL;
		}

		char* newpath = String::Clone(path);
		if ( !newpath ) { Error::Set(ENOSPC); return NULL; }

		DevRAMFSFile* file = new DevRAMFSFile(newpath);
		if ( !file ) { delete[] newpath; Error::Set(ENOSPC); return NULL; }
		if ( !files->Add(file) ) { delete file; Error::Set(ENOSPC); return NULL; }

		file->Refer();

		return file;
	}

	bool DevRAMFS::Unlink(const char* path)
	{
		if ( *path == '\0' || ( *path++ == '/' && *path == '\0' ) )
		{
			Error::Set(EISDIR);
			return false;
		}

		if ( !files ) { Error::Set(ENOENT); return false; }
		size_t index = files->Search(LookupFile, path);
		if ( index == SIZE_MAX ) { Error::Set(ENOENT); return false; }

		Device* dev = files->Remove(index);
		ASSERT(dev);
		dev->Unref();
		return true;
	}

	const bool BINDEVHACK = true;

	size_t DevRAMFS::GetNumFiles()
	{
		size_t result = 2 + (BINDEVHACK ? 2 : 0);
		if ( files ) { result += files->Length(); }
		return result;
	}

	const char* DevRAMFS::GetFilename(size_t index)
	{
		switch ( index )
		{
		case 0: return ".";
		case 1: return "..";
		default: index -= 2;
		}
		if ( BINDEVHACK ) switch ( index )
		{
		case 0: return "bin";
		case 1: return "dev";
		default: index -= 2;
		}
		if ( !files )
			return NULL;
		if ( files->Length() <= index )
			return NULL;
		DevRAMFSFile* file = files->Get(index);
		return file->name;
	}
}

