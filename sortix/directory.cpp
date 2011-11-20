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
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include "syscall.h"
#include "process.h"
#include "device.h"
#include "directory.h"

using namespace Maxsi;

namespace Sortix
{
	namespace Directory
	{
		int SysReadDirEnts(int fd, sortix_dirent* dirent, size_t size)
		{
			Process* process = CurrentProcess();
			Device* dev =  process->descriptors.Get(fd);
			if ( !dev ) { Error::Set(Error::EBADF); return -1; }
			if ( !dev->IsType(Device::DIRECTORY) ) { Error::Set(Error::EBADF); return -1; }
			DevDirectory* dir = (DevDirectory*) dev;

			sortix_dirent* prev = NULL;

			while ( true )
			{
				// Check if we got enough bytes left to tell how many bytes we
				// actually needed.
				if ( size < sizeof(sortix_dirent) )
				{
					if ( prev ) { return 0; } // We did some work.
					Error::Set(Error::EINVAL); // Nope, userspace was cheap.
					return -1;
				}

				// Attempt to read into the space left, and if we managed to
				// read at least one record, just say we succeded. The user
				// space buffer is empty on the next call, so that'll probably
				// succeed. The directory read function will store the number of
				// bytes needed in the d_namelen variable and set errno to
				// EINVAL such that userspace knows we need a larger buffer.
				if ( dir->Read(dirent, size) ) { return (prev) ? 0 : -1; }

				// Insert the current dirent into the single-linked list for
				// easy iteration by userspace.
				prev->d_next = dirent;
				dirent->d_next = NULL;

				// Check for end-of-directory conditions. Signal to userspace
				// that we are done by giving them the empty filename.
				if ( dirent->d_namelen == 0 ) { return 0; }

				// Alright, we managed to read a dirent. Now let's try reading
				// another one (provide as many as we can).
				prev = dirent;
				size_t bytesused = sizeof(sortix_dirent) + dirent->d_namelen + 1;
				size -= bytesused;
				dirent = (sortix_dirent*) ( ((byte*) dirent) + bytesused );
			}
		}

		void Init()
		{
			Syscall::Register(SYSCALL_READDIRENTS, (void*) SysReadDirEnts);
		}
	}
}


