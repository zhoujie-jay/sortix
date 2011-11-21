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

	fs/devfs.cpp
	Provides access to various block, character, and other kinds of devices.

******************************************************************************/

#include "../platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "../filesystem.h"
#include "../directory.h"
#include "../stream.h"
#include "devfs.h"

using namespace Maxsi;

namespace Sortix
{
	class DevLogTTY : public DevStream
	{
	public:
		typedef DevStream BaseClass;

	public:
		DevLogTTY();
		virtual ~DevLogTTY();

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};
	
	DevLogTTY::DevLogTTY()
	{
	}

	DevLogTTY::~DevLogTTY()
	{
		// TODO: Make an array of waiting threads on the keyboard..
	}

	ssize_t DevLogTTY::Read(byte* /*dest*/, size_t /*count*/)
	{
		// TODO: Read from keyboard.
		return 0;
	}

	ssize_t DevLogTTY::Write(const byte* src, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		
		return Log::deviceCallback(Log::devicePointer, (char*) src, count);
	}

	bool DevLogTTY::IsReadable()
	{
		return true;
	}

	bool DevLogTTY::IsWritable()
	{
		return true;
	}

	class DevNull : public DevStream
	{
	public:
		typedef DevStream BaseClass;

	public:
		DevNull();
		virtual ~DevNull();

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	};
	
	DevNull::DevNull()
	{
	}

	DevNull::~DevNull()
	{
	}

	ssize_t DevNull::Read(byte* /*dest*/, size_t /*count*/)
	{
		return 0; // Return EOF
	}

	ssize_t DevNull::Write(const byte* /*src*/, size_t count)
	{
		if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
		
		// O' glorious bitbucket in the sky, I hereby sacrifice to You, my holy
		// data in trust You will keep it safe. That You will store it for all
		// eternity, until the day You will return to User-Land to rule the land
		// and preserve data-integrity for all eternity. Amen.

		return count;
	}

	bool DevNull::IsReadable()
	{
		return true;
	}

	bool DevNull::IsWritable()
	{
		return true;
	}

	class DevDevFSDir : public DevDirectory
	{
	public:
		typedef Device DevDirectory;

	public:
		DevDevFSDir();
		virtual ~DevDevFSDir();

	public:
		size_t position;

	public:
		virtual void Rewind();
		virtual int Read(sortix_dirent* dirent, size_t available);

	};

	DevDevFSDir::DevDevFSDir()
	{
		position = 0;
	}

	DevDevFSDir::~DevDevFSDir()
	{
	}

	void DevDevFSDir::Rewind()
	{
		position = 0;
	}

	int DevDevFSDir::Read(sortix_dirent* dirent, size_t available)
	{
		const char* names[] = { "null", "tty" };
		const size_t nameslength = 2;

		if ( available <= sizeof(sortix_dirent) ) { return -1; }
		if ( nameslength <= position )
		{
			dirent->d_namelen = 0;
			dirent->d_name[0] = 0;
			return 0;
		}

		const char* name = names[position];
		size_t namelen = String::Length(name);
		size_t needed = sizeof(sortix_dirent) + namelen + 1;

		if ( available < needed )
		{
			dirent->d_namelen = needed;
			Error::Set(Error::EINVAL);
			return 0;
		}

		Memory::Copy(dirent->d_name, name, namelen + 1);
		dirent->d_namelen = namelen;
		position++;
		return 0;
	}

	DevDevFS::DevDevFS()
	{
	}

	DevDevFS::~DevDevFS()
	{
	}

	Device* DevDevFS::Open(const char* path, int flags, mode_t mode)
	{
		if ( (flags & O_LOWERFLAGS) == O_SEARCH )
		{
			if ( path[0] == 0 || (path[0] == '/' && path[1] == 0) ) { return new DevDevFSDir(); }
			Error::Set(Error::ENOTDIR);
			return NULL;
		}

		if ( String::Compare(path, "/null") == 0 ) { return new DevNull; }
		if ( String::Compare(path, "/tty") == 0 ) { return new DevLogTTY; }

		Error::Set(Error::ENOENT);
		return NULL;
	}

	bool DevDevFS::Unlink(const char* path)
	{
		if ( *path == '\0' || ( *path++ == '/' && *path == '\0' ) )
		{
			Error::Set(Error::EISDIR);
			return false;
		}

		Error::Set(Error::EPERM);
		return true;
	}
}

