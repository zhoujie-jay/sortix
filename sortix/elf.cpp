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

	elf.cpp
	Constructs processes from ELF files.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/mman.h>
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "elf.h"
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/panic.h>
#include "process.h"

using namespace Maxsi;

namespace Sortix
{
	namespace ELF
	{
		int ToProgramSectionType(int flags)
		{
			switch ( flags & (PF_X | PF_R | PF_W) )
			{
				case 0:
					return SEG_NONE;
				case PF_X:
				case PF_X | PF_R:
				case PF_X | PF_W:
				case PF_X | PF_R | PF_W:
					return SEG_TEXT;
				case PF_R:
				case PF_W:
				case PF_R | PF_W:
				default:
					return SEG_DATA;
			}
		}

		addr_t Construct32(Process* process, const void* file, size_t filelen)
		{
			if ( filelen < sizeof(Header32) ) { return 0; }
			const Header32* header = (const Header32*) file;

			// Check for little endian.
			if ( header->dataencoding != DATA2LSB ) { return 0; }
			if ( header->version != CURRENTVERSION ) { return 0; }

			addr_t entry = header->entry;

			// Find the location of the program headers.
			addr_t phtbloffset = header->programheaderoffset;
			if ( filelen < phtbloffset ) { return 0; }
			addr_t phtblpos = ((addr_t) file) + phtbloffset;
			size_t phsize = header->programheaderentrysize;
			const ProgramHeader32* phtbl = (const ProgramHeader32*) phtblpos;

			// Validate that all program headers are present.
			uint16_t numprogheaders = header->numprogramheaderentries;
			size_t neededfilelen = phtbloffset + numprogheaders * phsize;
			if ( filelen < neededfilelen ) { return 0; }

			// Prepare the process for execution (clean up address space, etc.)
			process->ResetForExecute();

			// Flush the TLB such that no stale information from the last
			// address space is used when creating the new one.
			Memory::Flush();

			// Create all the segments in the final process.
			// TODO: Handle errors on bad/malicious input or out-of-mem!
			for ( uint16_t i = 0; i < numprogheaders; i++ )
			{
				const ProgramHeader32* pht = &(phtbl[i]);
				if ( pht->type != PT_LOAD ) { continue; }
				addr_t virtualaddr = pht->virtualaddr;
				addr_t mapto = Page::AlignDown(virtualaddr);
				addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
				ASSERT(pht->offset % pht->align == virtualaddr % pht->align);
				ASSERT(pht->offset + pht->filesize < filelen);
				ASSERT(pht->filesize <= pht->memorysize);

				ProcessSegment* segment = new ProcessSegment;
				if ( segment == NULL ) { return 0; }
				segment->position = mapto;
				segment->size = Page::AlignUp(mapbytes);
				segment->type = ToProgramSectionType(pht->flags);

				int prot = PROT_FORK | PROT_KREAD | PROT_KWRITE;
				if ( pht->flags & PF_X ) { prot |= PROT_EXEC; }
				if ( pht->flags & PF_R ) { prot |= PROT_READ; }
				if ( pht->flags & PF_W ) { prot |= PROT_WRITE; }

				if ( segment->Intersects(process->segments) )
				{
					delete segment;
					return 0;
				}

				if ( !Memory::MapRange(mapto, mapbytes, prot))
				{
					// TODO: Memory leak of segment?
					return 0;
				}

				// Insert our newly allocated memory into the processes segment
				// list such that it can be reclaimed later.
				if ( process->segments ) { process->segments->prev = segment; }
				segment->next = process->segments;
				process->segments = segment;

				// Copy as much data as possible and memset the rest to 0.
				byte* memdest = (byte*) virtualaddr;
				byte* memsource = (byte*) ( ((addr_t)file) + pht->offset);
				Maxsi::Memory::Copy(memdest, memsource, pht->filesize);
				Maxsi::Memory::Set(memdest + pht->filesize, 0, pht->memorysize - pht->filesize);
			}

			return entry;
		}

		addr_t Construct64(Process* process, const void* file, size_t filelen)
		{
			#ifndef PLATFORM_X64
			Error::Set(ENOEXEC);
			return 0;
			#else
			if ( filelen < sizeof(Header64) ) { return 0; }
			const Header64* header = (const Header64*) file;

			// Check for little endian.
			if ( header->dataencoding != DATA2LSB ) { return 0; }
			if ( header->version != CURRENTVERSION ) { return 0; }

			addr_t entry = header->entry;

			// Find the location of the program headers.
			addr_t phtbloffset = header->programheaderoffset;
			if ( filelen < phtbloffset ) { return 0; }
			addr_t phtblpos = ((addr_t) file) + phtbloffset;
			size_t phsize = header->programheaderentrysize;
			const ProgramHeader64* phtbl = (const ProgramHeader64*) phtblpos;

			// Validate that all program headers are present.
			uint16_t numprogheaders = header->numprogramheaderentries;
			size_t neededfilelen = phtbloffset + numprogheaders * phsize;
			if ( filelen < neededfilelen ) { return 0; }

			// Prepare the process for execution (clean up address space, etc.)
			process->ResetForExecute();

			// Flush the TLB such that no stale information from the last
			// address space is used when creating the new one.
			Memory::Flush();

			// Create all the segments in the final process.
			// TODO: Handle errors on bad/malicious input or out-of-mem!
			for ( uint16_t i = 0; i < numprogheaders; i++ )
			{
				const ProgramHeader64* pht = &(phtbl[i]);
				if ( pht->type != PT_LOAD ) { continue; }
				addr_t virtualaddr = pht->virtualaddr;
				addr_t mapto = Page::AlignDown(virtualaddr);
				addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
				ASSERT(pht->offset % pht->align == virtualaddr % pht->align);
				ASSERT(pht->offset + pht->filesize < filelen);
				ASSERT(pht->filesize <= pht->memorysize);

				ProcessSegment* segment = new ProcessSegment;
				if ( segment == NULL ) { return 0; }
				segment->position = mapto;
				segment->size = Page::AlignUp(mapbytes);
				segment->type = ToProgramSectionType(pht->flags);

				int prot = PROT_FORK | PROT_KREAD | PROT_KWRITE;
				if ( pht->flags & PF_X ) { prot |= PROT_EXEC; }
				if ( pht->flags & PF_R ) { prot |= PROT_READ; }
				if ( pht->flags & PF_W ) { prot |= PROT_WRITE; }

				if ( segment->Intersects(process->segments) )
				{
					delete segment;
					return 0;
				}

				if ( !Memory::MapRange(mapto, mapbytes, prot))
				{
					// TODO: Memory leak of segment?
					return 0;
				}

				// Insert our newly allocated memory into the processes segment
				// list such that it can be reclaimed later.
				if ( process->segments ) { process->segments->prev = segment; }
				segment->next = process->segments;
				process->segments = segment;

				// Copy as much data as possible and memset the rest to 0.
				byte* memdest = (byte*) virtualaddr;
				byte* memsource = (byte*) ( ((addr_t)file) + pht->offset);
				Maxsi::Memory::Copy(memdest, memsource, pht->filesize);
				Maxsi::Memory::Set(memdest + pht->filesize, 0, pht->memorysize - pht->filesize);
			}

			return entry;
			#endif
		}

		addr_t Construct(Process* process, const void* file, size_t filelen)
		{
			if ( filelen < sizeof(Header) ) { Error::Set(ENOEXEC); return 0; }
			const Header* header = (const Header*) file;

			if ( !(header->magic[0] == 0x7F && header->magic[1] == 'E' &&
                   header->magic[2] == 'L'  && header->magic[3] == 'F'  ) )
			{
				Error::Set(ENOEXEC);
				return 0;
			}

			switch ( header->fileclass )
			{
				case CLASS32:
					return Construct32(process, file, filelen);
				case CLASS64:
					return Construct64(process, file, filelen);
				default:
					return 0;
			}
		}
	}
}

