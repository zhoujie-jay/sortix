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

	initrd.h
	Declares the structure of the Sortix ramdisk.

******************************************************************************/

#ifndef SORTIX_INITRD_H
#define SORTIX_INITRD_H

namespace Sortix
{
	namespace InitRD
	{
		struct Header;
		struct FileHeader;

		struct Header
		{
			char magic[16]; // Contains "sortix-initrd-1"
			uint32_t numfiles;
			// FileHeader[numfiles];
		};

		struct FileHeader
		{
			mode_t permissions;
			uid_t owner;
			gid_t group;
			uint32_t size;
			uint32_t offset; // where the physical data is located.
			char name[128];
		};

#ifdef SORTIX_KERNEL
		void Init(byte* initrd, size_t size);
		byte* Open(const char* filepath, size_t* size);
		const char* GetFilename(size_t index);
		size_t GetNumFiles();
#endif
	}
}

#endif
