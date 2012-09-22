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

	fs/util.cpp
	Utility classes for kernel filesystems.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/string.h>
#include <errno.h>
#include <string.h>
#include "util.h"

using namespace Maxsi;

namespace Sortix {

DevStringBuffer::DevStringBuffer(char* str)
{
	this->str = str;
	strlength = strlen(str);
	off = 0;
}

DevStringBuffer::~DevStringBuffer()
{
	delete[] str;
}

size_t DevStringBuffer::BlockSize()
{
	return 1;
}

uintmax_t DevStringBuffer::Size()
{
	return strlength;
}

uintmax_t DevStringBuffer::Position()
{
	return off;
}

bool DevStringBuffer::Seek(uintmax_t position)
{
	if ( strlength <= position ) { errno = EINVAL; return false; }
	off = position;
	return true;
}

bool DevStringBuffer::Resize(uintmax_t size)
{
	if ( size != strlength ) { errno = EBADF; }
	return false;
}

ssize_t DevStringBuffer::Read(uint8_t* dest, size_t count)
{
	size_t available = strlength - off;
	if ( available < count ) { count = available; }
	memcpy(dest, str + off, count);
	off += count;
	return count;
}

ssize_t DevStringBuffer::Write(const uint8_t* /*src*/, size_t /*count*/)
{
	errno = EBADF;
	return -1;
}

bool DevStringBuffer::IsReadable()
{
	return true;
}

bool DevStringBuffer::IsWritable()
{
	return false;
}


DevLineCommand::DevLineCommand(bool (*handler)(void*, const char*), void* user)
{
	this->handler = handler;
	this->user = user;
	this->handled = false;
	this->sofar = 0;
}

DevLineCommand::~DevLineCommand()
{
}

ssize_t DevLineCommand::Read(uint8_t* /*dest*/, size_t /*count*/)
{
	errno = EBADF;
	return -1;
}

ssize_t DevLineCommand::Write(const uint8_t* src, size_t count)
{
	if ( handled ) { errno = EINVAL; return -1; }
	size_t available = CMDMAX - sofar;
	if ( !available && count ) { errno = ENOSPC; return -1; }
	if ( available < count ) { count = available; }
	memcpy(cmd + sofar, src, count);
	cmd[sofar += count] = 0;
	size_t newlinepos = String::Reject(cmd, "\n");
	if ( !cmd[newlinepos] ) { return count; }
	cmd[newlinepos] = 0;
	if ( !handler(user, cmd) ) { return -1; }
	return count;
}

bool DevLineCommand::IsReadable()
{
	return false;
}

bool DevLineCommand::IsWritable()
{
	return true;
}


DevMemoryBuffer::DevMemoryBuffer(uint8_t* buf, size_t bufsize, bool write,
	                             bool deletebuf)
{
	this->buf = buf;
	this->bufsize = bufsize;
	this->write = write;
	this->deletebuf = deletebuf;
}

DevMemoryBuffer::~DevMemoryBuffer()
{
	if ( deletebuf )
		delete[] buf;
}

size_t DevMemoryBuffer::BlockSize()
{
	return 1;
}

uintmax_t DevMemoryBuffer::Size()
{
	return bufsize;
}

uintmax_t DevMemoryBuffer::Position()
{
	return off;
}

bool DevMemoryBuffer::Seek(uintmax_t position)
{
	off = position;
	return true;
}

bool DevMemoryBuffer::Resize(uintmax_t size)
{
	if ( size != bufsize ) { errno = EPERM; return false; }
	return true;
}

ssize_t DevMemoryBuffer::Read(uint8_t* dest, size_t count)
{
	if ( bufsize <= off ) { return 0; }
	size_t available = bufsize - off;
	if ( available < count ) { count = available; }
	memcpy(dest, buf + off, count);
	off += count;
	return count;
}

ssize_t DevMemoryBuffer::Write(const uint8_t* src, size_t count)
{
	if ( !write ) { errno = EBADF; return -1; }
	if ( bufsize <= off ) { errno = EPERM; return -1; }
	size_t available = bufsize - off;
	if ( available < count ) { count = available; }
	memcpy(buf + off, src, count);
	off += count;
	return count;
}

bool DevMemoryBuffer::IsReadable()
{
	return true;
}

bool DevMemoryBuffer::IsWritable()
{
	return write;
}

} // namespace Sortix

