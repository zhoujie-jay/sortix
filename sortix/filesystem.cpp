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

	filesystem.cpp
	Allows access to stored sequences of bytes in an orderly fashion.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "process.h"
#include "filesystem.h"
#include "mount.h"

namespace Sortix
{
	namespace FileSystem
	{
		Device* Open(const char* path, int flags, mode_t mode)
		{
			size_t pathoffset = 0;
			DevFileSystem* fs = Mount::WhichFileSystem(path, &pathoffset);
			if ( !fs ) { return NULL; }
			return fs->Open(path + pathoffset, flags, mode); 
		}

		int SysOpen(const char* path, int flags, mode_t mode)
		{
			Process* process = CurrentProcess();
			Device* dev = Open(path, flags, mode);
			if ( !dev ) { return -1; /* TODO: errno */ }
			int fd = process->descriptors.Allocate(dev);
			if ( fd < 0 ) { dev->Unref(); }
			return fd;			
		}

		void Init()
		{
			Syscall::Register(SYSCALL_OPEN, (void*) SysOpen);
		}
	}
}


