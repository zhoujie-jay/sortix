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

	elf.h
	Constructs processes from ELF files.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "elf.h"
#include "memorymanagement.h"

#include "log.h" // DEBUG

namespace Sortix
{
	namespace ELF
	{
		addr_t Construct32(const void* file, size_t filelen)
		{
			if ( filelen < sizeof(Header32) ) { return 0; }
			const Header32* header = (const Header32*) file;

			// Check for little endian.
			if ( header->dataencoding != DATA2LSB ) { return 0; }
			if ( header->version != CURRENTVERSION ) { return 0; }

			addr_t entry = header->entry;

			// Find the location of the first section header.
			addr_t shtbloffset = header->sectionheaderoffset;
			addr_t shtblpos = ((addr_t) file) + shtbloffset;
			size_t shsize = header->sectionheaderentrysize;

			// Validate that all sections are present.
			uint16_t numsections = header->numsectionheaderentries;
			size_t neededfilelen = shtbloffset + numsections * shsize;
			if ( filelen < neededfilelen ) { return 0; } 

			addr_t memlower = SIZE_MAX;
			addr_t memupper = 0;

			// Valid all the data we need is in the file.
			for ( uint16_t i = 0; i < numsections; i++ )
			{
				addr_t shoffset = i * shsize;
				addr_t shpos = shtblpos + shoffset;
				const SectionHeader32* sh = (const SectionHeader32*) shpos;
				if ( sh->addr == 0 ) { continue; }
				if ( sh->type == SHT_PROGBITS )
				{
					size_t sectionendsat = sh->offset + sh->size;
					if ( filelen < sectionendsat ) { return 0; }
				}
				if ( sh->type == SHT_PROGBITS || sh->type == SHT_NOBITS )
				{
					if ( sh->addr < memlower ) { memlower = sh->addr; }
					if ( memupper < sh->addr + sh->size ) { memupper = sh->addr + sh->size; }
				}
			}

			if ( memupper < memlower ) { return entry; }

			// HACK: For now just put it in the same continious block. This is
			// very wasteful and not very intelligent.
			addr_t mapto = Page::AlignDown(memlower);
			addr_t mapbytes = memupper - mapto;
			// TODO: Validate that we actually may map here!
			if ( !VirtualMemory::MapRangeUser(mapto, mapbytes) )
			{
				return 0;
			}

			// Create all the sections in the final process.
			for ( uint16_t i = 0; i < numsections; i++ )
			{
				addr_t shoffset = i * shsize;
				addr_t shpos = shtblpos + shoffset;
				const SectionHeader32* sh = (const SectionHeader32*) shpos;
				if ( sh->type != SHT_PROGBITS && sh->type != SHT_NOBITS )
				{
					continue;
				}

				if ( sh->addr == 0 ) { continue; }

				if ( sh->type == SHT_PROGBITS )
				{
					void* memdest = (void*) sh->addr;
					void* memsource = (void*) ( ((addr_t) file) + sh->offset );
					Maxsi::Memory::Copy(memdest, memsource, sh->size);
				}
			}

			// MEMORY LEAK: There is no system in place to delete the sections
			// once the process has terminated.

			return entry;
		}

		addr_t Construct64(const void* /*file*/, size_t /*filelen*/)
		{
			return 0;
		}

		addr_t Construct(const void* file, size_t filelen)
		{
			if ( filelen < sizeof(Header) ) { return 0; }
			const Header* header = (const Header*) file;

			if ( !(header->magic[0] == 0x7F && header->magic[1] == 'E' &&
                   header->magic[2] == 'L'  && header->magic[3] == 'F'  ) )
			{
				return 0;
			}

			switch ( header->fileclass )
			{
				case CLASS32:
					return Construct32(file, filelen);
				case CLASS64:
					return Construct64(file, filelen);
				default:
					return 0;
			}		
		}
	}
}

