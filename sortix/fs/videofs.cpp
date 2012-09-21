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

	fs/videofs.h
	Provides filesystem access to the video framework.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/video.h>
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include "../directory.h"
#include "util.h"
#include "videofs.h"

using namespace Maxsi;

namespace Sortix {

class DevFrameBuffer : public DevBuffer
{
public:
	DevFrameBuffer();
	virtual ~DevFrameBuffer();

private:
	uintmax_t off;

public:
	virtual size_t BlockSize();
	virtual uintmax_t Size();
	virtual uintmax_t Position();
	virtual bool Seek(uintmax_t position);
	virtual bool Resize(uintmax_t size);
	virtual ssize_t Read(uint8_t* dest, size_t count);
	virtual ssize_t Write(const uint8_t* src, size_t count);
	virtual bool IsReadable();
	virtual bool IsWritable();

};

DevFrameBuffer::DevFrameBuffer()
{
	off = 0;
}

DevFrameBuffer::~DevFrameBuffer()
{
}

size_t DevFrameBuffer::BlockSize()
{
	return 1; // Well, not really.
}

uintmax_t DevFrameBuffer::Size()
{
	return Video::FrameSize();
}

uintmax_t DevFrameBuffer::Position()
{
	return off;
}

bool DevFrameBuffer::Seek(uintmax_t position)
{
	off = position;
	return true;
}

bool DevFrameBuffer::Resize(uintmax_t /*size*/)
{
	Error::Set(EBADF);
	return false;
}

ssize_t DevFrameBuffer::Read(uint8_t* dest, size_t count)
{
	ssize_t result = Video::ReadAt(off, dest, count);
	if ( 0 <= result ) { off += result; }
	return result;
}

ssize_t DevFrameBuffer::Write(const uint8_t* src, size_t count)
{
	ssize_t result = Video::WriteAt(off, src, count);
	if ( 0 <= result ) { off += result; }
	return result;
}

bool DevFrameBuffer::IsReadable()
{
	return true;
}

bool DevFrameBuffer::IsWritable()
{
	return true;
}

bool SetModeHandler(void* /*user*/, const char* cmd)
{
	return Video::SwitchMode(cmd);
}

bool SupportsModeHandler(void* /*user*/, const char* cmd)
{
	return Video::Supports(cmd);
}

Device* MakeGetMode(int /*flags*/, mode_t /*mode*/)
{
	char* mode = Video::GetCurrentMode();
	if ( !mode ) { return NULL; }
	char* modeline = String::Combine(2, mode, "\n");
	delete[] mode; mode = NULL;
	if ( !modeline ) { return NULL; }
	Device* result = new DevStringBuffer(modeline);
	if ( !result ) { delete[] modeline; return NULL; }
	return result;
}

Device* MakeSetMode(int /*flags*/, mode_t /*mode*/)
{
	return new DevLineCommand(SetModeHandler, NULL);
}

Device* MakeMode(int flags, mode_t mode)
{
	int lowerflags = flags & O_LOWERFLAGS;
	if ( lowerflags == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }
	if ( lowerflags == O_RDONLY ) { return MakeGetMode(flags, mode); }
	if ( lowerflags == O_WRONLY ) { return MakeSetMode(flags, mode); }
	Error::Set(EPERM);
	return NULL;
}

Device* MakeModes(int flags, mode_t /*mode*/)
{
	int lowerflags = flags & O_LOWERFLAGS;
	if ( lowerflags == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }
	if ( lowerflags != O_RDONLY ) { Error::Set(EPERM); return NULL; }
	size_t nummodes = 0;
	char** modes = Video::GetModes(&nummodes);
	if ( !modes ) { return NULL; }
	Device* result = NULL;
	size_t combinedlen = 0;
	for ( size_t i = 0; i < nummodes; i++ )
	{
		combinedlen += String::Length(modes[i]) + 1 /*newline*/;
	}
	size_t sofar = 0;
	char* modesstr = new char[combinedlen + 1];
	if ( !modesstr ) { goto out; }
	for ( size_t i = 0; i < nummodes; i++ )
	{
		String::Copy(modesstr + sofar, modes[i]);
		sofar += String::Length(modes[i]);
		modesstr[sofar++] = '\n';
	}
	modesstr[sofar] = 0;
	result = new DevStringBuffer(modesstr);
	if ( !result ) { delete[] modesstr; }
out:
	for ( size_t i = 0; i < nummodes; i++ ) { delete[] modes[i]; }
	delete[] modes;
	return result;
}

Device* MakeSupports(int flags, mode_t /*mode*/)
{
	int lowerflags = flags & O_LOWERFLAGS;
	if ( lowerflags == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }
	return new DevLineCommand(SupportsModeHandler, NULL);
}

Device* MakeFB(int flags, mode_t /*mode*/)
{
	int lowerflags = flags & O_LOWERFLAGS;
	if ( lowerflags == O_SEARCH ) { Error::Set(ENOTDIR); return NULL; }
	return new DevFrameBuffer();
}

Device* MakeDot(int /*flags*/, mode_t /*mode*/)
{
	return NULL;
}

Device* MakeDotDot(int /*flags*/, mode_t /*mode*/)
{
	return NULL;
}

struct
{
	const char* name;
	Device* (*factory)(int, mode_t);
} nodes[] =
{
	{ ".", MakeDot },
	{ "..", MakeDotDot },
	{ "mode", MakeMode },
	{ "modes", MakeModes },
	{ "supports", MakeSupports },
	{ "fb", MakeFB },
};

static inline size_t NumNodes()
{
	return sizeof(nodes)/sizeof(nodes[0]);
}

class DevVideoFSDir : public DevDirectory
{
public:
	DevVideoFSDir();
	virtual ~DevVideoFSDir();

private:
	size_t position;

public:
	virtual void Rewind();
	virtual int Read(sortix_dirent* dirent, size_t available);

};

DevVideoFSDir::DevVideoFSDir()
{
	position = 0;
}

DevVideoFSDir::~DevVideoFSDir()
{
}

void DevVideoFSDir::Rewind()
{
	position = 0;
}

int DevVideoFSDir::Read(sortix_dirent* dirent, size_t available)
{
	if ( available <= sizeof(sortix_dirent) ) { return -1; }
	if ( NumNodes() <= position )
	{
		dirent->d_namelen = 0;
		dirent->d_name[0] = 0;
		return 0;
	}

	const char* name = nodes[position].name;
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

DevVideoFS::DevVideoFS()
{
}

DevVideoFS::~DevVideoFS()
{
}

Device* DevVideoFS::Open(const char* path, int flags, mode_t mode)
{
	if ( !String::Compare(path, "") || !String::Compare(path, "/") )
	{
		if ( (flags & O_LOWERFLAGS) == O_SEARCH ) { return new DevVideoFSDir; }
		Error::Set(EISDIR);
		return NULL;
	}

	if ( *path++ != '/' ) { Error::Set(ENOENT); return NULL; }

	for ( size_t i = 0; i < NumNodes(); i++ )
	{
		if ( String::Compare(path, nodes[i].name) ) { continue; }
		return nodes[i].factory(flags, mode);
	}
	Error::Set(ENOENT);
	return NULL;
}

bool DevVideoFS::Unlink(const char* /*path*/)
{
	Error::Set(EPERM);
	return false;
}

} // namespace Sortix
