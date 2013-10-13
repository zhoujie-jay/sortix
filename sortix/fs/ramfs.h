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

	fs/ramfs.h
	A filesystem stored entirely in RAM.

******************************************************************************/

#ifndef SORTIX_FS_RAMFS_H
#define SORTIX_FS_RAMFS_H

#include <sortix/kernel/sortedlist.h>
#include "../filesystem.h"

namespace Sortix
{
	class DevRAMFSFile;

	class DevRAMFS : public DevFileSystem
	{
	public:
		DevRAMFS();
		virtual ~DevRAMFS();

	public:
		virtual Device* Open(const char* path, int flags, mode_t mode);
		virtual bool Unlink(const char* path);

	private:
		SortedList<DevRAMFSFile*>* files;

	public:
		size_t GetNumFiles();
		const char* GetFilename(size_t index);

	private:
		virtual DevBuffer* OpenFile(const char* path, int flags, mode_t mode);
		virtual DevBuffer* CreateFile(const char* path, int flags, mode_t mode);

	};
}

#endif
