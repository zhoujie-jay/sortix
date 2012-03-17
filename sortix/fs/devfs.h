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

	fs/devfs.h
	Provides access to various block, character, and other kinds of devices.

******************************************************************************/

#ifndef SORTIX_FS_DEVFS_H
#define SORTIX_FS_DEVFS_H

#include <libmaxsi/sortedlist.h>
#include "../filesystem.h"

namespace Sortix
{
	class ATADrive;

	class DevDevFS : public DevFileSystem
	{
	public: 
		DevDevFS();
		virtual ~DevDevFS();

	public:
		virtual Device* Open(const char* path, int flags, mode_t mode);
		virtual bool Unlink(const char* path);

	};

	namespace DeviceFS {

	struct DevEntry
	{
		char* name;
		Device* dev;
	};

	void Init();
	void RegisterATADrive(unsigned ataid, ATADrive* drive);
	bool RegisterDevice(const char* name, Device* dev);
	Device* LookUp(const char* name);
	size_t GetNumDevices();
	DevEntry* GetDevice(size_t index);

	} // namespace DeviceFS
}

#endif

