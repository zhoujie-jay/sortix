/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	fs/util.h
	Utility classes for kernel filesystems.

*******************************************************************************/

#ifndef SORTIX_FS_UTIL_H
#define SORTIX_FS_UTIL_H

#include "../stream.h"

namespace Sortix {

class DevStringBuffer : public DevBuffer
{
public:
	DevStringBuffer(char* str);
	virtual ~DevStringBuffer();

private:
	char* str;
	size_t strlength;
	size_t off;

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

class DevLineCommand : public DevStream
{
public:
	DevLineCommand(bool (*handler)(void*, const char*), void* user);
	virtual ~DevLineCommand();
	virtual ssize_t Read(byte* dest, size_t count);
	virtual ssize_t Write(const byte* src, size_t count);
	virtual bool IsReadable();
	virtual bool IsWritable();

private:
	bool (*handler)(void*, const char*);
	void* user;
	size_t sofar;
	static const size_t CMDMAX = 255;
	char cmd[CMDMAX+1];
	bool handled;

};

} // namespace Sortix

#endif
