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
	Abstracts away a file system device.

******************************************************************************/

#include "device.h"
#include "thread.h"
#include "process.h"
#include "filesystem.h"

namespace Sortix
{
	bool DevFileSystem::LegalNodeName(const char* name)
	{
		if ( *name == '\0' ) { return false; }
		while ( *name != '\0' )
		{
			if ( *name == '/' ) { return false; }
		}
		return true;
	}

	bool DevFileSystem::Open(const char* path, nat openFlags, nat mode, Mount::SysCallback* callbackInfo, Thread* thread)
	{
#if 0
		nat requestMode = flags & O_LOWERFLAGS;

		MXFSNode* node;

		if ( flags & O_CREAT )
		{
			if ( flags & O_DIRECTORY )	{ return CreateDirectory(path, flags, permissions, type); }
			else						{ return CreateFile(path, flags, permissions, type); }
		}
		else
		{
			node = Search(path);
		}

		if ( node == NULL ) { return NULL; }

		if ( node->IsDir() )
		{
			if ( requestMode == O_SEARCH || requestMode == O_RDONLY )
			{
				// TODO: Make a directory device.
				Error::Set(Error::NOTIMPLEMENTED);
				return NULL;
			}
			else
			{
				Error::Set(Error::ISDIR);
				return NULL;
			}
		}

		if ( 
#endif
	}
}

