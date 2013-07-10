/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    directory.h
    A container for files and other directories.

*******************************************************************************/

#ifndef SORTIX_DIRECTORY_H
#define SORTIX_DIRECTORY_H

#include "device.h"

namespace Sortix
{
	// Keep this up to date with <sys/readdirents.h>
	struct sortix_dirent
	{
		struct sortix_dirent* d_next;
		unsigned char d_type;
		size_t d_namelen;
		char d_name[];
	};

	class DevDirectory : public Device
	{
	public:
		typedef Device BaseClass;

	public:
		virtual void Rewind() = 0;

		// Precondition: available is at least sizeof(sortix_dirent).
		virtual int Read(sortix_dirent* dirent, size_t available) = 0;

	public:
		virtual bool IsType(unsigned type) const { return type == Device::DIRECTORY; }

	};

	namespace Directory
	{
		void Init();
		char* MakeAbsolute(const char* wd, const char* rel);
	}
}

#endif
