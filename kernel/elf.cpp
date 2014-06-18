/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    elf.cpp
    Constructs processes from ELF files.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/elf-note.h>
#include <sortix/mman.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>
#include <sortix/kernel/symbol.h>

#include "elf.h"

namespace Sortix {
namespace ELF {

// TODO: This code doesn't respect that the size of program headers and section
//       headers may vary depending on the ELF header and that using a simple
//       table indexation isn't enough.

addr_t Construct32(Process* process, const uint8_t* file, size_t filelen,
                   Auxiliary* aux)
{
	if ( filelen < sizeof(Header32) )
		return 0;
	const Header32* header = (const Header32*) file;

	// Check for little endian.
	if ( header->dataencoding != DATA2LSB )
		return 0;
	if ( header->version != CURRENTVERSION )
		return 0;

	addr_t entry = header->entry;

	// Find the location of the program headers.
	addr_t phtbloffset = header->programheaderoffset;
	if ( filelen < phtbloffset )
		return 0;
	addr_t phtblpos = ((addr_t) file) + phtbloffset;
	size_t phsize = header->programheaderentrysize;
	const ProgramHeader32* phtbl = (const ProgramHeader32*) phtblpos;

	// Validate that all program headers are present.
	uint16_t numprogheaders = header->numprogramheaderentries;
	size_t neededfilelen = phtbloffset + numprogheaders * phsize;
	if ( filelen < neededfilelen )
		return 0;

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
		if ( pht->type == PT_TLS )
		{
			aux->tls_file_offset = pht->offset;
			aux->tls_file_size = pht->filesize;
			aux->tls_mem_size = pht->memorysize;
			aux->tls_mem_align = pht->align;
			continue;
		}
		if ( pht->type == PT_NOTE )
		{
			uintptr_t notes_addr = (uintptr_t) file + pht->offset;
			size_t notes_offset = 0;
			while ( notes_offset < pht->filesize )
			{
				uintptr_t note = notes_addr + notes_offset;
				uint32_t namesz = *(uint32_t*) (note + 0);
				uint32_t descsz = *(uint32_t*) (note + 4);
				uint32_t type = *(uint32_t*) (note + 8);
				uint32_t namesz_aligned = -(-namesz & ~(4U - 1));
				uint32_t descsz_aligned = -(-descsz & ~(4U - 1));
				size_t note_size = 12 + namesz_aligned + descsz_aligned;
				notes_offset += note_size;
				const char* name = (const char*) (note + 12);
				uintptr_t desc = note + 12 + namesz_aligned;
				if ( strcmp(name, "Sortix") == 0 )
				{
					if ( type == ELF_NOTE_SORTIX_UTHREAD_SIZE )
					{
						aux->uthread_size = *(uint32_t*) (desc + 0);
						aux->uthread_align = *(uint32_t*) (desc + 4);
					}
				}
			}
			continue;
		}
		if ( pht->type != PT_LOAD )
			continue;
		addr_t virtualaddr = pht->virtualaddr;
		addr_t mapto = Page::AlignDown(virtualaddr);
		addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
		assert(pht->offset % pht->align == virtualaddr % pht->align);
		assert(pht->offset + pht->filesize < filelen);
		assert(pht->filesize <= pht->memorysize);

		int prot = PROT_FORK | PROT_KREAD | PROT_KWRITE;
		if ( pht->flags & PF_X ) { prot |= PROT_EXEC; }
		if ( pht->flags & PF_R ) { prot |= PROT_READ; }
		if ( pht->flags & PF_W ) { prot |= PROT_WRITE; }

		struct segment segment;
		segment.addr = mapto;
		segment.size = Page::AlignUp(mapbytes);
		segment.prot = prot;

		kthread_mutex_lock(&process->segment_lock);

		if ( !IsUserspaceSegment(&segment) ||
		     IsSegmentOverlapping(process, &segment) )
		{
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		assert(process == CurrentProcess());

		if ( !Memory::MapRange(segment.addr, segment.size, prot, PAGE_USAGE_USER_SPACE) )
		{
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		if ( !AddSegment(process, &segment) )
		{
			Memory::UnmapRange(segment.addr, segment.size, PAGE_USAGE_USER_SPACE);
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		kthread_mutex_unlock(&process->segment_lock);

		// Copy as much data as possible and memset the rest to 0.
		uint8_t* memdest = (uint8_t*) virtualaddr;
		uint8_t* memsource = (uint8_t*) (((addr_t)file) + pht->offset);
		memcpy(memdest, memsource, pht->filesize);
		memset(memdest + pht->filesize, 0, pht->memorysize - pht->filesize);
	}

	// Find the location of the section headers.
	addr_t shtblpos = (addr_t) file + header->sectionheaderoffset;
	const SectionHeader32* shtbl = (const SectionHeader32*) shtblpos;

	const SectionHeader32* section_names_section = shtbl + header->sectionheaderstringindex;
	const char* section_names = (const char*) (file + section_names_section->offset);

	// Find the string table.
	const SectionHeader32* string_table_section = NULL;
	for ( size_t i = 0; i < header->numsectionheaderentries; i++ )
		if ( !strcmp(section_names + shtbl[i].name, ".strtab") )
		{
			string_table_section = shtbl + i;
			break;
		}

	// Find the symbol table.
	const SectionHeader32* symbol_table_section = NULL;
	for ( size_t i = 0; i < header->numsectionheaderentries; i++ )
		if ( !strcmp(section_names + shtbl[i].name, ".symtab") )
		{
			symbol_table_section = shtbl + i;
			break;
		}

	if ( !string_table_section || !symbol_table_section )
		return entry;

	// Prepare copying debug information.
	const char* elf_string_table = (const char*) (file + string_table_section->offset);
	size_t elf_string_table_size = string_table_section->size;
	const Symbol32* elf_symbols = (const Symbol32*) (file + symbol_table_section->offset);
	size_t elf_symbol_count = symbol_table_section->size / sizeof(Symbol32);

	// Duplicate the string table.
	char* string_table = new char[elf_string_table_size];
	if ( !string_table )
		return entry;
	memcpy(string_table, elf_string_table, elf_string_table_size);

	// Duplicate the symbol table.
	Symbol* symbol_table = new Symbol[elf_symbol_count-1];
	if ( !symbol_table )
	{
		delete[] string_table;
		return entry;
	}

	// Copy all entires except the leading null entry.
	for ( size_t i = 1; i < elf_symbol_count; i++ )
	{
		symbol_table[i-1].address = elf_symbols[i].st_value;
		symbol_table[i-1].size = elf_symbols[i].st_size;
		symbol_table[i-1].name = string_table + elf_symbols[i].st_name;
	}

	process->string_table = string_table;
	process->string_table_length = elf_string_table_size;
	process->symbol_table = symbol_table;
	process->symbol_table_length = elf_symbol_count-1;

	return entry;
}

addr_t Construct64(Process* process, const uint8_t* file, size_t filelen,
                   Auxiliary* aux)
{
#if !defined(__x86_64__)
	(void) process;
	(void) file;
	(void) filelen;
	(void) aux;
	return errno = ENOEXEC, 0;
#else
	if ( filelen < sizeof(Header64) )
		return 0;
	const Header64* header = (const Header64*) file;

	// Check for little endian.
	if ( header->dataencoding != DATA2LSB )
		return 0;
	if ( header->version != CURRENTVERSION )
		return 0;

	addr_t entry = header->entry;

	// Find the location of the program headers.
	addr_t phtbloffset = header->programheaderoffset;
	if ( filelen < phtbloffset )
		return 0;
	addr_t phtblpos = ((addr_t) file) + phtbloffset;
	size_t phsize = header->programheaderentrysize;
	const ProgramHeader64* phtbl = (const ProgramHeader64*) phtblpos;

	// Validate that all program headers are present.
	uint16_t numprogheaders = header->numprogramheaderentries;
	size_t neededfilelen = phtbloffset + numprogheaders * phsize;
	if ( filelen < neededfilelen )
		return 0;

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
		if ( pht->type == PT_TLS )
		{
			aux->tls_file_offset = pht->offset;
			aux->tls_file_size = pht->filesize;
			aux->tls_mem_size = pht->memorysize;
			aux->tls_mem_align = pht->align;
			continue;
		}
		if ( pht->type == PT_NOTE )
		{
			uintptr_t notes_addr = (uintptr_t) file + pht->offset;
			size_t notes_offset = 0;
			while ( notes_offset < pht->filesize )
			{
				uintptr_t note = notes_addr + notes_offset;
				uint32_t namesz = *(uint32_t*) (note + 0);
				uint32_t descsz = *(uint32_t*) (note + 4);
				uint32_t type = *(uint32_t*) (note + 8);
				uint32_t namesz_aligned = -(-namesz & ~(4U - 1));
				uint32_t descsz_aligned = -(-descsz & ~(4U - 1));
				size_t note_size = 12 + namesz_aligned + descsz_aligned;
				notes_offset += note_size;
				const char* name = (const char*) (note + 12);
				uintptr_t desc = note + 12 + namesz_aligned;
				if ( strcmp(name, "Sortix") == 0 )
				{
					if ( type == ELF_NOTE_SORTIX_UTHREAD_SIZE )
					{
						uint64_t size_low = *((uint32_t*) (desc + 0));
						uint64_t size_high = *((uint32_t*) (desc + 4));
						aux->uthread_size = size_low << 0 | size_high << 32;
						uint64_t align_low = *((uint32_t*) (desc + 8));
						uint64_t align_high = *((uint32_t*) (desc + 12));
						aux->uthread_align = align_low << 0 | align_high << 32;
					}
				}
			}
			continue;
		}
		if ( pht->type != PT_LOAD )
			continue;
		addr_t virtualaddr = pht->virtualaddr;
		addr_t mapto = Page::AlignDown(virtualaddr);
		addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
		assert(pht->offset % pht->align == virtualaddr % pht->align);
		assert(pht->offset + pht->filesize < filelen);
		assert(pht->filesize <= pht->memorysize);

		int prot = PROT_FORK | PROT_KREAD | PROT_KWRITE;
		if ( pht->flags & PF_X ) { prot |= PROT_EXEC; }
		if ( pht->flags & PF_R ) { prot |= PROT_READ; }
		if ( pht->flags & PF_W ) { prot |= PROT_WRITE; }

		struct segment segment;
		segment.addr = mapto;
		segment.size = Page::AlignUp(mapbytes);
		segment.prot = prot;

		kthread_mutex_lock(&process->segment_lock);

		if ( !IsUserspaceSegment(&segment) ||
		     IsSegmentOverlapping(process, &segment) )
		{
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		assert(process == CurrentProcess());

		if ( !Memory::MapRange(segment.addr, segment.size, prot, PAGE_USAGE_USER_SPACE) )
		{
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		if ( !AddSegment(process, &segment) )
		{
			Memory::UnmapRange(segment.addr, segment.size, PAGE_USAGE_USER_SPACE);
			kthread_mutex_unlock(&process->segment_lock);
			process->ResetAddressSpace();
			return 0;
		}

		kthread_mutex_unlock(&process->segment_lock);

		// Copy as much data as possible and memset the rest to 0.
		uint8_t* memdest = (uint8_t*) virtualaddr;
		uint8_t* memsource = (uint8_t*) (((addr_t)file) + pht->offset);
		memcpy(memdest, memsource, pht->filesize);
		memset(memdest + pht->filesize, 0, pht->memorysize - pht->filesize);
	}

	// Find the location of the section headers.
	addr_t shtblpos = (addr_t) file + header->sectionheaderoffset;
	const SectionHeader64* shtbl = (const SectionHeader64*) shtblpos;

	const SectionHeader64* section_names_section = shtbl + header->sectionheaderstringindex;
	const char* section_names = (const char*) (file + section_names_section->offset);

	// Find the string table.
	const SectionHeader64* string_table_section = NULL;
	for ( size_t i = 0; i < header->numsectionheaderentries; i++ )
		if ( !strcmp(section_names + shtbl[i].name, ".strtab") )
		{
			string_table_section = shtbl + i;
			break;
		}

	// Find the symbol table.
	const SectionHeader64* symbol_table_section = NULL;
	for ( size_t i = 0; i < header->numsectionheaderentries; i++ )
		if ( !strcmp(section_names + shtbl[i].name, ".symtab") )
		{
			symbol_table_section = shtbl + i;
			break;
		}

	if ( !string_table_section || !symbol_table_section )
		return entry;

	// Prepare copying debug information.
	const char* elf_string_table = (const char*) (file + string_table_section->offset);
	size_t elf_string_table_size = string_table_section->size;
	const Symbol64* elf_symbols = (const Symbol64*) (file + symbol_table_section->offset);
	size_t elf_symbol_count = symbol_table_section->size / sizeof(Symbol64);

	// Duplicate the string table.
	char* string_table = new char[elf_string_table_size];
	if ( !string_table )
		return entry;
	memcpy(string_table, elf_string_table, elf_string_table_size);

	// Duplicate the symbol table.
	Symbol* symbol_table = new Symbol[elf_symbol_count-1];
	if ( !symbol_table )
	{
		delete[] string_table;
		return entry;
	}

	// Copy all entires except the leading null entry.
	for ( size_t i = 1; i < elf_symbol_count; i++ )
	{
		symbol_table[i-1].address = elf_symbols[i].st_value;
		symbol_table[i-1].size = elf_symbols[i].st_size;
		symbol_table[i-1].name = string_table + elf_symbols[i].st_name;
	}

	process->string_table = string_table;
	process->string_table_length = elf_string_table_size;
	process->symbol_table = symbol_table;
	process->symbol_table_length = elf_symbol_count-1;

	return entry;
#endif
}

addr_t Construct(Process* process, const void* file, size_t filelen,
                 Auxiliary* aux)
{
	if ( filelen < sizeof(Header) )
		return errno = ENOEXEC, 0;

	const Header* header = (const Header*) file;

	if ( !(header->magic[0] == 0x7F && header->magic[1] == 'E' &&
           header->magic[2] == 'L'  && header->magic[3] == 'F'  ) )
		return errno = ENOEXEC, 0;

	memset(aux, 0, sizeof(*aux));

	switch ( header->fileclass )
	{
		case CLASS32: return Construct32(process, (const uint8_t*) file, filelen, aux);
		case CLASS64: return Construct64(process, (const uint8_t*) file, filelen, aux);
		default:
			return 0;
	}
}

} // namespace ELF
} // namespace Sortix
