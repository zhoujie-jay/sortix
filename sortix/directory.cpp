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
#include "filesystem.h"
#include "mount.h"

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

		int SysChDir(const char* path)
		{
			// Calculate the absolute new path.
			Process* process = CurrentProcess();
			const char* wd = process->workingdir;
			char* abs = MakeAbsolute(wd, path);
			if ( !abs ) { Error::Set(Error::ENOMEM); return -1; }
			size_t abslen = String::Length(abs);
			if ( 1 < abslen && abs[abslen-1] == '/' )
			{
				abs[abslen-1] = '\0';
			}

			// Lookup the path and see if it is a directory.
			size_t pathoffset = 0;
			DevFileSystem* fs = Mount::WhichFileSystem(abs, &pathoffset);
			if ( !fs ) { delete[] abs; Error::Set(Error::EINVAL); return -1; }
			Device* dev = fs->Open(abs + pathoffset, O_SEARCH | O_DIRECTORY, 0);
			if ( !dev ) { Error::Set(Error::ENOTDIR); return -1; }
			dev->Unref();

			// Alright, the path passed.
			delete[] process->workingdir; // Works if it was NULL.
			process->workingdir = abs;

			return 0;
		}

		char* SysGetCWD(char* buf, size_t size)
		{
			// Calculate the absolute new path.
			Process* process = CurrentProcess();
			const char* wd = process->workingdir;
			if ( !wd ) { wd = "/"; }
			size_t wdsize = String::Length(wd) + 1;
			if ( size < wdsize ) { Error::Set(Error::ERANGE); return NULL; }
			String::Copy(buf, wd);
			return buf;
		}

		void Init()
		{
			Syscall::Register(SYSCALL_READDIRENTS, (void*) SysReadDirEnts);
			Syscall::Register(SYSCALL_CHDIR, (void*) SysChDir);
			Syscall::Register(SYSCALL_GETCWD, (void*) SysGetCWD);
		}

		// Allocate a byte too much, in case you want to add a trailing slash.
		char* MakeAbsolute(const char* wd, const char* rel)
		{
			// If given no wd, then interpret from the root.
			if ( !wd ) { wd = "/"; }

			// The resulting size won't ever be larger than this.
			size_t wdlen = String::Length(wd);
			size_t resultsize = wdlen + String::Length(rel) + 2;
			char* result = new char[resultsize + 1];
			if ( !result ) { return NULL; }

			// Detect if rel is relative to / or to wd, and then continue the
			// interpretation from that point.
			size_t offset;
			if ( *rel == '/' ) { result[0] = '/'; offset = 1; rel++; }
			else { String::Copy(result, wd); offset = wdlen; }

			// Make sure the working directory ends with a slash.
			if ( result[offset-1] != '/' ) { result[offset++] = '/'; }

			bool leadingdots = true;
			size_t dots = 0;
			int c = 1;
			while ( c ) // Exit after handling \0
			{
				c = *rel++;

				// Don't insert double //'s into the final path.
				if ( c == '/' && result[offset-1] == '/' ) { continue; }

				// / or \0 means that we should interpret . and ..
				if ( c == '/' || c == '\0' )
				{
					// If ., just remove the dot and ignore the slash.
					if ( leadingdots && dots == 1 )
					{
						result[--offset] = '\0';
						dots = 0;
						continue;
					}

					// If .., remove .. and one element of the path.
					if ( leadingdots && dots == 2 )
					{
						offset -= 2; // Remove ..
						offset -= 1; // Remove the trailing slash
						while ( offset )
						{
							if ( result[--offset] == '/' ) { break; }
						}
						result[offset++] = '/'; // Need to re-insert a slash.
						result[offset] = '\0';
						dots = 0;
						continue;
					}

					// Reset the dot count after a slash.
					dots = 0;
				}

				// The newest path element consisted solely of dots.
				leadingdots = ( c == '/' || c == '.' );

				// Count the number of leading dots in the path element.
				if ( c == '.' && leadingdots ) { dots++; }

				// Insert the character into the result.
				result[offset++] = c;
			}

			// TODO: To avoid wasting space, should we String::Clone(result)?

			return result;
		}
	}
}


