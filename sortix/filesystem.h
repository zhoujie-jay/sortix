/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    filesystem.h
    Allows access to stored sequences of bytes in an orderly fashion.

*******************************************************************************/

#ifndef SORTIX_FILESYSTEM_H
#define SORTIX_FILESYSTEM_H

#include "device.h"
#include "stream.h"
#include "fcntl.h"

namespace Sortix
{
	class DevFileSystem : public Device
	{
	public:
		virtual Device* Open(const char* path, int flags, mode_t mode) = 0;
		virtual bool Unlink(const char* path) = 0;

	public:
		virtual bool IsType(unsigned type) const { return type == Device::FILESYSTEM; }

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
		virtual ssize_t Read(uint8_t* dest, size_t count);
		virtual ssize_t Write(const uint8_t* src, size_t count);
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
