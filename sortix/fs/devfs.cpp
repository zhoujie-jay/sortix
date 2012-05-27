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

#include <sortix/kernel/platform.h>
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
#include "videofs.h"

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

	bool DevATA::Resize(uintmax_t /*size*/)
	{
		Error::Set(EPERM);
		return false;
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

	// TODO: Move this namespace into something like devicefs.cpp.
	namespace DeviceFS {

	size_t entriesused;
	size_t entrieslength;
	DevEntry* deventries;
	DevVideoFS* videofs;

	void Init()
	{
		deventries = NULL;
		entriesused = 0;
		entrieslength = 0;
		videofs = new DevVideoFS;
		if ( !videofs ) { Panic("Unable to allocate videofs\n"); }
	}

	bool RegisterDevice(const char* name, Device* dev)
	{
		if ( entriesused == entrieslength )
		{
			size_t newentrieslength = entrieslength ? 2 * entrieslength : 32;
			DevEntry* newdeventries = new DevEntry[newentrieslength];
			if ( !newdeventries ) { return false; }
			size_t bytes = sizeof(DevEntry) * entriesused;
			Memory::Copy(newdeventries, deventries, bytes);
			delete[] deventries;
			entrieslength = newentrieslength;
			deventries = newdeventries;
		}

		char* nameclone = String::Clone(name);
		if ( !nameclone ) { return false; }

		size_t index = entriesused++;
		dev->Refer();
		deventries[index].name = nameclone;
		deventries[index].dev = dev;
		return true;
	}

	Device* LookUp(const char* name)
	{
		for ( size_t i = 0; i < entriesused; i++ )
		{
			if ( String::Compare(name, deventries[i].name) ) { continue; }
			deventries[i].dev->Refer();
			return deventries[i].dev;
		}
		return NULL;
	}

	size_t GetNumDevices()
	{
		return entriesused;
	}

	DevEntry* GetDevice(size_t index)
	{
		if ( entriesused <= index ) { return NULL; }
		return deventries + index;
	}

	// TODO: Hack to register ATA devices.
	// FIXME: Move class DevATA into ata.cpp.
	void RegisterATADrive(unsigned ataid, ATADrive* drive)
	{
		DevATA* ata = new DevATA(drive);
		if ( !ata ) { Panic("Cannot allocate ATA device"); }
		ata->Refer();
		ASSERT(ataid < 10);
		char name[5] = "ataN";
		name[3] = '0' + ataid;
		if ( !RegisterDevice(name, ata) )
		{
			PanicF("Cannot register /dev/%s", name);
		}
		ata->Unref();
	}

	} // namespace DeviceFS

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
		const char* names[] = { "null", "tty", "video", "vga" };
		const char* name = NULL;
		if ( position < DeviceFS::GetNumDevices() )
		{
			name = DeviceFS::GetDevice(position)->name;
		}
		else
		{
			const size_t nameslength = 4;
			size_t index = position - DeviceFS::GetNumDevices();
			if ( nameslength <= index )
			{
				dirent->d_namelen = 0;
				dirent->d_name[0] = 0;
				return 0;
			}
			name = names[index];
		}

		if ( available <= sizeof(sortix_dirent) ) { return -1; }

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
		if ( String::Compare(path, "/video") == 0 ||
             String::StartsWith(path, "/video/") )
		{
			return DeviceFS::videofs->Open(path + String::Length("/video"), flags, mode);
		}

		Device* dev = DeviceFS::LookUp(path + 1);
		if ( !dev )
		{
			Error::Set(flags & O_CREAT ? EPERM : ENOENT);
			return NULL;
		}
		if ( dev->IsType(Device::BUFFER) )
		{
			DevBuffer* buffer = (DevBuffer*) dev;
			DevFileWrapper* wrapper = new DevFileWrapper(buffer, flags);
			buffer->Unref();
			return wrapper;
		}
		return dev;
	}

	bool DevDevFS::Unlink(const char* path)
	{
		if ( String::Compare(path, "/video") == 0 ||
             String::StartsWith(path, "/video/") )
		{
			return DeviceFS::videofs->Unlink(path);
		}

		if ( *path == '\0' || ( *path++ == '/' && *path == '\0' ) )
		{
			Error::Set(EISDIR);
			return false;
		}

		Error::Set(EPERM);
		return false;
	}
}

