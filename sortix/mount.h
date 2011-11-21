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

	mount.h
	Handles system wide mount points and initialization of new file systems.

******************************************************************************/

#ifndef SORTIX_MOUNT_H
#define SORTIX_MOUNT_H

namespace Sortix
{
	class DevFileSystem;

	namespace Mount
	{
		void Init();
		DevFileSystem* WhichFileSystem(const char* path, size_t* pathoffset);
		bool Register(DevFileSystem* fs, const char* path);
	}
}

#endif

