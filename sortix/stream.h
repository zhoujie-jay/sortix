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

	stream.h
	Various device types that provides a sequence of bytes.

******************************************************************************/

#ifndef SORTIX_STREAM_H
#define SORTIX_STREAM_H

#include "device.h"

namespace Sortix
{
	class DevStream : public Device
	{
	public:
		typedef Device BaseClass;

	public:
		virtual bool IsType(unsigned type) const { return type == Device::STREAM; }

	public:
		virtual ssize_t Read(uint8_t* dest, size_t count) = 0;
		virtual ssize_t Write(const uint8_t* src, size_t count) = 0;
		virtual bool IsReadable() = 0;
		virtual bool IsWritable() = 0;

	};

	class DevBuffer : public DevStream
	{
	public:
		typedef DevStream BaseClass;

	public:
		virtual bool IsType(unsigned type) const
		{
			return type == Device::BUFFER || BaseClass::IsType(type);
		}

	public:
		virtual size_t BlockSize() = 0;
		virtual uintmax_t Size() = 0;
		virtual uintmax_t Position() = 0;
		virtual bool Seek(uintmax_t position) = 0;
		virtual bool Resize(uintmax_t size) = 0;

	};
}

#endif
