/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	initrd.h
	Provides low-level access to a Sortix init ramdisk.

*******************************************************************************/

#ifndef SORTIX_INITRD_KERNEL_H
#define SORTIX_INITRD_KERNEL_H

namespace Sortix {
namespace InitRD {

void Init(addr_t phys, size_t size);
uint32_t Root();
bool Stat(uint32_t inode, struct stat* st);
uint8_t* Open(uint32_t inode, size_t* size);
uint32_t Traverse(uint32_t inode, const char* name);
const char* GetFilename(uint32_t dir, size_t index);
size_t GetNumFiles(uint32_t dir);

} // namespace InitRD
} // namespace Sortix

#endif
