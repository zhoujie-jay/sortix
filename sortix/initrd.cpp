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

	initrd.cpp
	Declares the structure of the Sortix ramdisk.

******************************************************************************/

#include "platform.h"
#include "initrd.h"
#include <libmaxsi/string.h>

#include "log.h" // DEBUG

using namespace Maxsi;

namespace Sortix
{
	namespace InitRD
	{
		byte* initrd;
		size_t initrdsize;

		void Init(byte* theinitrd, size_t size)
		{
			initrd = theinitrd;
			initrdsize = size;
			if ( size < sizeof(Header) ) { PanicF("initrd.cpp: initrd is too small"); }
			Header* header = (Header*) initrd;
			if ( String::Compare(header->magic, "sortix-initrd-1") != 0 ) { PanicF("initrd.cpp: invalid magic value in the initrd"); }
			size_t sizeneeded = sizeof(Header) + header->numfiles * sizeof(FileHeader);
			if ( size < sizeneeded ) { PanicF("initrd.cpp: initrd is too small"); }
			// TODO: We need to do more validation here!
		}

		byte* Open(const char* filepath, size_t* size)
		{
			Header* header = (Header*) initrd;
			FileHeader* fhtbl = (FileHeader*) (initrd + sizeof(Header));
			for ( uint32_t i = 0; i < header->numfiles; i++ )
			{
				FileHeader* fileheader = &(fhtbl[i]);

				if ( String::Compare(filepath, fileheader->name) != 0 ) { continue; }

				*size = fileheader->size;
				return initrd + fileheader->offset;
			}

			return NULL;
		}
	}
}
