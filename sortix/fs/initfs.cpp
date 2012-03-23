/******************************************************************************

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

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	fs/initfs.h
	Provides access to the initial ramdisk.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "../filesystem.h"
#include "../directory.h"
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
		this->offset = 0;
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
		if ( SIZE_MAX < position ) { Error::Set(EOVERFLOW); return false; }
		offset = position;
		return true;
	}

	bool DevInitFSFile::Resize(uintmax_t /*size*/)
	{
		Error::Set(EBADF);
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
		Error::Set(EBADF);
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

	class DevInitFSDir : public DevDirectory
	{
	public:
		typedef Device DevDirectory;

	public:
		DevInitFSDir(uint32_t dir);
		virtual ~DevInitFSDir();

	private:
		size_t position;
		uint32_t dir;
		uint32_t numfiles;

	public:
		virtual void Rewind();
		virtual int Read(sortix_dirent* dirent, size_t available);

	};

	DevInitFSDir::DevInitFSDir(uint32_t dir)
	{
		this->position = 0;
		this->dir = dir;
		this->numfiles = InitRD::GetNumFiles(dir);
	}

	DevInitFSDir::~DevInitFSDir()
	{
	}

	void DevInitFSDir::Rewind()
	{
		position = 0;
	}

	int DevInitFSDir::Read(sortix_dirent* dirent, size_t available)
	{
		if ( available <= sizeof(sortix_dirent) ) { return -1; }
		if ( numfiles <= position )
		{
			dirent->d_namelen = 0;
			dirent->d_name[0] = 0;
			return 0;
		}

		const char* name = InitRD::GetFilename(dir, position);
		size_t namelen = String::Length(name);
		size_t needed = sizeof(sortix_dirent) + namelen + 1;

		// Oh right, the kernel is stupid and doesn't support dot and dotdot.
		if ( name[0] == '.' ) { position++; return Read(dirent, available); }

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

	DevInitFS::DevInitFS()
	{
	}

	DevInitFS::~DevInitFS()
	{
	}

	Device* DevInitFS::Open(const char* path, int flags, mode_t mode)
	{
		size_t buffersize;
		int lowerflags = flags & O_LOWERFLAGS;

		if ( !path[0] || (path[0] == '/' && !path[1]) )
		{
			if ( lowerflags != O_SEARCH ) { Error::Set(EISDIR); return NULL; }
			return new DevInitFSDir(InitRD::Root());
		}

		if ( *path++ != '/' ) { Error::Set(ENOENT); return NULL; }

		uint32_t ino = InitRD::Traverse(InitRD::Root(), path);
		if ( !ino ) { return NULL; }

		const byte* buffer = InitRD::Open(ino, &buffersize);
		if ( !buffer ) { return NULL; }

		if ( lowerflags == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }
		if ( lowerflags != O_RDONLY ) { Error::Set(EROFS); return NULL; }

		char* newpath = String::Clone(path);
		if ( !newpath ) { Error::Set(ENOSPC); return NULL; }

		Device* result = new DevInitFSFile(newpath, buffer, buffersize);
		if ( !result ) { delete[] newpath; Error::Set(ENOSPC); return NULL; }

		return result;
	}

	bool DevInitFS::Unlink(const char* path)
	{
		Error::Set(EROFS);
		return false;
	}
}

