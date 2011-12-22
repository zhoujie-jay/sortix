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
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include "syscall.h"
#include "memorymanagement.h"

#include "log.h" // DEBUG

using namespace Maxsi;

namespace Sortix
{
	namespace InitRD
	{
		byte* initrd;
		size_t initrdsize;
#ifdef JSSORTIX
		// JSVM never tells JSSortix how big the initrd is!
		const bool CHECK_CHECKSUM = false;
#else
		const bool CHECK_CHECKSUM = true;
#endif

		size_t GetNumFiles()
		{
			Header* header = (Header*) initrd;
			return header->numfiles;
		}
		const char* GetFilename(size_t index)
		{
			Header* header = (Header*) initrd;
			if ( index >= header->numfiles ) { return NULL; }
			FileHeader* fhtbl = (FileHeader*) (initrd + sizeof(Header));
			FileHeader* fileheader = &(fhtbl[index]);
			return fileheader->name;
		}

		uint8_t ContinueChecksum(uint8_t checksum,  const void* p, size_t size)
		{
			const uint8_t* buffer = (const uint8_t*) p;
			while ( size-- )
			{
				checksum += *buffer++;
			}
			return checksum;
		}

		void CheckSum()
		{
			Trailer* trailer = (Trailer*) (initrd + initrdsize - sizeof(Trailer));
			uint8_t checksum = ContinueChecksum(0, initrd, initrdsize - sizeof(Trailer));
			if ( trailer->sum != checksum )
			{
				PanicF("InitRD Checksum failed: the ramdisk may have been "
				       "corrupted by the bootloader: Got %u instead of %u "
				       "when checking the ramdisk at 0x%p + 0x%zx bytes\n",
				       checksum, trailer->sum, initrd, initrdsize);
			}
		}

		void Init(addr_t phys, size_t size)
		{
			// First up, map the initrd onto the kernel's address space.
			addr_t virt = Memory::GetInitRD();
			size_t amount = 0;
			while ( amount < size )
			{
				if ( !Memory::MapKernel(phys + amount, virt + amount) )
				{
					Panic("Unable to map the init ramdisk into virtual memory");
				}
				amount += 0x1000UL;
			}

			Memory::Flush();

			initrd = (byte*) virt;
			initrdsize = size;
			if ( size < sizeof(Header) ) { PanicF("initrd.cpp: initrd is too small"); }
			Header* header = (Header*) initrd;
			if ( String::Compare(header->magic, "sortix-initrd-1") != 0 ) { PanicF("initrd.cpp: invalid magic value in the initrd"); }
			size_t sizeneeded = sizeof(Header) + header->numfiles * sizeof(FileHeader);
			if ( size < sizeneeded ) { PanicF("initrd.cpp: initrd is too small"); }
			// TODO: We need to do more validation here!

			if ( CHECK_CHECKSUM ) { CheckSum(); }
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
