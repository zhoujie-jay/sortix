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
	Allows access to stored sequences of bytes in an orderly fashion.

******************************************************************************/

#ifndef SORTIX_FILESYSTEM_H
#define SORTIX_FILESYSTEM_H

#include "device.h"
#include "stream.h"

namespace Sortix
{
	// TODO: These belong in libmaxsi!
	// TODO: Sortix might never support all of these flags if they are stupid.
	const int O_RDONLY = 1;
	const int O_WRONLY = 2;
	const int O_RDWR = 3;
	const int O_EXEC = 4;
	const int O_SEARCH = 5;
	const int O_LOWERFLAGS = 0x7;
	const int O_APPEND = (1<<3);
	const int O_CLOEXEC = (1<<4);
	const int O_CREAT = (1<<5);
	const int O_DIRECTORY = (1<<6);
	const int O_DSYNC = (1<<6);
	const int O_EXCL = (1<<7);
	const int O_NOCTTY = (1<<8);
	const int O_NOFOLLOW = (1<<9);
	const int O_RSYNC = (1<<11);
	const int O_SYNC = (1<<12);
	const int O_TRUNC = (1<<13);
	const int O_TTY_INIT = (1<<13);

	class DevFileSystem : public Device
	{
	public:
		virtual Device* Open(const char* path, int flags, mode_t mode) = 0;
		virtual bool Unlink(const char* path) = 0;

	public:
		virtual bool IsType(unsigned type) { return type == Device::FILESYSTEM; }

	};

	class DevFileWrapper : public DevBuffer
	{
	public:
		DevFileWrapper(DevBuffer* buffer, int flags);
		virtual ~DevFileWrapper();

	private:
		DevBuffer* buffer;
		int flags;
		uintmax_t offset;

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

	namespace FileSystem
	{
		void Init();
		Device* Open(const char* path, int flags, mode_t mode);
		bool Unlink(const char* path);
	}
}

#endif

