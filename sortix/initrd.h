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

#ifndef SORTIX_INITRD_KERNEL_H
#define SORTIX_INITRD_KERNEL_H

namespace Sortix
{
	namespace InitRD
	{
		void Init(addr_t phys, size_t size);
		byte* Open(const char* filepath, size_t* size);
		const char* GetFilename(size_t index);
		size_t GetNumFiles();
	}
}

#endif
