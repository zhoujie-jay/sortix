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
#include "../terminal.h"
#include "../vga.h"
#include "../ata.h"
#include "devfs.h"

using namespace Maxsi;

namespace Sortix
{
	class DevATA : public DevBuffer
	{
	public:
		typedef DevBuffer BaseClass;

	public:
		DevATA(ATADrive* drive);
		virtual ~DevATA();

	private:
		ATADrive* drive;
		off_t offset;

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();
		virtual size_t BlockSize();
		virtual uintmax_t Size();
		virtual uintmax_t Position();
		virtual bool Seek(uintmax_t position);
		virtual bool Resize(uintmax_t size);

	};

	DevATA::DevATA(ATADrive* drive)
	{
		this->drive = drive;
		offset = 0;
	}

	DevATA::~DevATA()
	{
	}

	ssize_t DevATA::Read(byte* dest, size_t count)
	{
		if ( SIZE_MAX < count ) { count = SIZE_MAX; }
		if ( drive->GetSize() - offset < count ) { count = drive->GetSize() - offset; }
		size_t amount = drive->Read(offset, dest, count);
		if ( count && !amount ) { return -1; }
		offset += amount;
		return amount;
	}

	ssize_t DevATA::Write(const byte* src, size_t count)
	{
		if ( SIZE_MAX < count ) { count = SIZE_MAX; }
		if ( drive->GetSize() <= offset && count ) { Error::Set(ENOSPC); return -1; }
		if ( drive->GetSize() - offset < count ) { count = drive->GetSize() - offset; }
		size_t amount = drive->Write(offset, src, count);
		if ( count && !amount ) { return -1; }
		offset += amount;
		return amount;
	}

	bool DevATA::IsReadable()
	{
		return true;
	}

	bool DevATA::IsWritable()
	{
		return true;
	}

	size_t DevATA::BlockSize()
	{
		return drive->GetSectorSize();
	}

	uintmax_t DevATA::Size()
	{
		return drive->GetSize();
	}

	uintmax_t DevATA::Position()
	{
		return offset;
	}

	bool DevATA::Seek(uintmax_t position)
	{
		if ( drive->GetSize() <= position ) { Error::Set(ENOSPC); return false; }
		offset = position;
		return true;
	}

	bool DevATA::Resize(uintmax_t size)
	{
		Error::Set(EPERM);
		return false;
	}

	const size_t NUM_ATAS = 4;
	DevATA* atalist[NUM_ATAS];

	void InitATADriveList()
	{
		for ( size_t i = 0; i < NUM_ATAS; i++ )
		{
			atalist[i] = NULL;
		}
	}

	void RegisterATADrive(unsigned ataid, ATADrive* drive)
	{
		atalist[ataid] = new DevATA(drive);
		atalist[ataid]->Refer();
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
		const char* names[] = { "null", "tty", "vga", "ata0", "ata1", "ata2", "ata3" };
		const size_t nameslength = 7;

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

		if ( name[0] == 'a' && name[1] == 't' && name[2] == 'a' )
		{
			if ( !atalist[name[3]-'0'] )
			{
				position++;
				return Read(dirent, available);
			}
		}

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

	DevDevFS::DevDevFS()
	{
	}

	DevDevFS::~DevDevFS()
	{
	}

	extern DevTerminal* tty;

	Device* DevDevFS::Open(const char* path, int flags, mode_t mode)
	{
		int lowerflags = flags & O_LOWERFLAGS;

		if ( !path[0] || (path[0] == '/' && !path[1]) )
		{
			if ( lowerflags != O_SEARCH ) { Error::Set(EISDIR); return NULL; }
			return new DevDevFSDir();
		}

		if ( String::Compare(path, "/null") == 0 ) { return new DevNull; }
		if ( String::Compare(path, "/tty") == 0 ) { tty->Refer(); return tty; }
		if ( String::Compare(path, "/vga") == 0 ) { return new DevVGA; }
		if ( path[0] == '/' && path[1] == 'a' && path[2] == 't' && path[3] == 'a' && path[4] && !path[5] )
		{
			if ( '0' <= path[4] && path[4] <= '9' )
			{
				size_t index = path[4] - '0';
				if  ( index <= NUM_ATAS && atalist[index] )
				{
					bool write = flags & O_WRONLY; // TODO: HACK: O_RDWD != O_WRONLY | O_RDONLY
					if ( write && !ENABLE_DISKWRITE )
					{
						Error::Set(EPERM);
						return false;
					}
					return new DevFileWrapper(atalist[index], flags);
				}
			}
		}

		Error::Set(flags & O_CREAT ? EPERM : ENOENT);
		return NULL;
	}

	bool DevDevFS::Unlink(const char* path)
	{
		if ( *path == '\0' || ( *path++ == '/' && *path == '\0' ) )
		{
			Error::Set(EISDIR);
			return false;
		}

		Error::Set(EPERM);
		return false;
	}
}

