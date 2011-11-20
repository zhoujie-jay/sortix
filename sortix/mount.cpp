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

	mount.cpp
	Handles system wide mount points and initialization of new file systems.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "panic.h"
#include "mount.h"
#include "fs/ramfs.h"
#include "fs/initfs.h"

namespace Sortix
{
	namespace Mount
	{
		DevFileSystem* initfs;
		DevFileSystem* rootfs;

		DevFileSystem* WhichFileSystem(const char* path, size_t* pathoffset)
		{
			if ( path[0] == '/' && path[1] == 'b' && path[2] == 'i' && path[3] == 'n' && (path[4] == '/' || path[4] == 0) )
			{
				*pathoffset = 4;
				return initfs;
			}
			*pathoffset = 0;
			return rootfs;
		}

		void Init()
		{
			initfs = new DevInitFS();
			if ( !initfs ) { Panic("Unable to allocate initfs"); }
			rootfs = new DevRAMFS();
			if ( !rootfs ) { Panic("Unable to allocate rootfs"); }
		}
	}
}
