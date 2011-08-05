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

namespace Sortix
{
	class Device
	{
	public:
		Device() { };
		virtual ~Device() { }

	public:
		virtual void close() = 0;

	};

	class DevStream : public Device
	{
	public:
		DevStream() { };
		virtual ~DevStream() { }

	public:
		virtual size_t write(const void* buffer, size_t bufferSize) = 0;
		virtual size_t readSome(const void* buffer, size_t bufferSize) = 0;

	public:
		inline size_t read(const void* buffer, size_t bufferSize)
		{
			const uint8_t* bytes = (uint8_t*) buffer;
			size_t total = bufferSize;
			while ( BufferSize > 0 )
			{
				size_t got = readSome(bytes, bufferSize);
				if ( got == SIZE_MAX ) { return SIZE_MAX; }
				bytes += got;
				bufferSize -= got;				
			}
			return total;
		}
	};

	class DevBuffer : public DevStream
	{
	public:
		DevBuffer() { };
		virtual ~DevBuffer() { }

	public:
		virtual size_t write(const void* buffer, size_t bufferSize) = 0;
		virtual size_t readSome(const void* buffer, size_t bufferSize) = 0;

	public:
		virtual size_t blockSize() = 0;
		virtual intmax_t size() = 0;
		virtual intmax_t position() = 0;
		virtual bool seek(intmax_t position) = 0;
		virtual bool resize(intmax_t size) = 0;

	};
}

#endif

