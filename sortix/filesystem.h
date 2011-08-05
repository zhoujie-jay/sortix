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

	filesystem.h
	Abstracts away a file system device.

******************************************************************************/

#ifndef SORTIX_FILESYSTEM_H
#define SORTIX_FILESYSTEM_H

#include "device.h"
#include "mount.h"

namespace Sortix
{
	class Thread;

	namespace FileSystem
	{
		struct SysCallback
		{
			Device* device;
			union { int deviceError; nat deviceType; }
		}
	}

	// TODO: These belong in libmaxsi!

	const nat O_RDONLY = 1;
	const nat O_WRONLY = 2;
	const nat O_RDWR = 3;
	const nat O_EXEC = 4;
	const nat O_SEARCH = 5;
	const nat O_LOWERFLAGS = 0x7;

	// TODO: Sortix might never support all of these flags if they are stupid.
	const nat O_APPEND = (1<<3);
	const nat O_CLOEXEC = (1<<4);
	const nat O_CREAT = (1<<5);
	const nat O_DIRECTORY = (1<<6);
	const nat O_DSYNC = (1<<6);
	const nat O_EXCL = (1<<7);
	const nat O_NOCTTY = (1<<8);
	const nat O_NOFOLLOW = (1<<9);
	const nat O_RSYNC = (1<<11);
	const nat O_SYNC = (1<<12);
	const nat O_TRUNC = (1<<13);
	const nat O_TTY_INIT = (1<<13);

	// If O_RDONLY, then no one is allowed to write to this, or if O_RDWD then
	// no one else is allowed to use this besides me.
	const nat O_EXCLUSIVELY = (1<<14);
	const nat O_USERSPACEABLE = ((1<<15)-1);

	const nat O_MOUNT = (1<<31);

	class DevFileSystem : public Device
	{
	public:
		DevFileSystem() { };
		virtual ~DevFileSystem() { }

	public:
		bool LegalNodeName(const char* name);

	public:
		virtual int Initialize(MountPoint* mountPoint, const char* commandLine) = 0;

	public:
		virtual bool Open(const char* path, nat openFlags, nat mode, FileSystem::SysCallback* callbackInfo, Thread* thread);

	};

	namespace FileSystem
	{
		DevFileSystem* CreateDriver(const char* fsType);
	}
}

#endif

