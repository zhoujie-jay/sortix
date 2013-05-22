/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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
#include <string.h>

#include <sortix/mman.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/symbol.h>

#include "elf.h"

namespace Sortix {
namespace ELF {

static int ToProgramSectionType(int flags)
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

// TODO: This code doesn't respect that the size of program headers and section
//       headers may vary depending on the ELF header and that using a simple
//       table indexation isn't enough.

addr_t Construct32(Process* process, const uint8_t* file, size_t filelen)
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
		if ( pht->type != PT_LOAD )
			continue;
		addr_t virtualaddr = pht->virtualaddr;
		addr_t mapto = Page::AlignDown(virtualaddr);
		addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
		assert(pht->offset % pht->align == virtualaddr % pht->align);
		assert(pht->offset + pht->filesize < filelen);
		assert(pht->filesize <= pht->memorysize);

		ProcessSegment* segment = new ProcessSegment;
		if ( segment == NULL )
			return 0;
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

		if ( !Memory::MapRange(mapto, mapbytes, prot) )
			// TODO: Memory leak of segment?
			return 0;

		// Insert our newly allocated memory into the processes segment
		// list such that it can be reclaimed later.
		if ( process->segments )
			process->segments->prev = segment;
		segment->next = process->segments;
		process->segments = segment;

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

addr_t Construct64(Process* process, const uint8_t* file, size_t filelen)
{
#ifndef PLATFORM_X64
	(void) process;
	(void) file;
	(void) filelen;
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
		if ( pht->type != PT_LOAD )
			continue;
		addr_t virtualaddr = pht->virtualaddr;
		addr_t mapto = Page::AlignDown(virtualaddr);
		addr_t mapbytes = virtualaddr - mapto + pht->memorysize;
		assert(pht->offset % pht->align == virtualaddr % pht->align);
		assert(pht->offset + pht->filesize < filelen);
		assert(pht->filesize <= pht->memorysize);

		ProcessSegment* segment = new ProcessSegment;
		if ( segment == NULL )
			return 0;
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

		if ( !Memory::MapRange(mapto, mapbytes, prot) )
		{
			// TODO: Memory leak of segment?
			return 0;
		}

		// Insert our newly allocated memory into the processes segment
		// list such that it can be reclaimed later.
		if ( process->segments )
			process->segments->prev = segment;
		segment->next = process->segments;
		process->segments = segment;

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

addr_t Construct(Process* process, const void* file, size_t filelen)
{
	if ( filelen < sizeof(Header) )
		return errno = ENOEXEC, 0;

	const Header* header = (const Header*) file;

	if ( !(header->magic[0] == 0x7F && header->magic[1] == 'E' &&
           header->magic[2] == 'L'  && header->magic[3] == 'F'  ) )
		return errno = ENOEXEC, 0;

	switch ( header->fileclass )
	{
		case CLASS32: return Construct32(process, (const uint8_t*) file, filelen);
		case CLASS64: return Construct64(process, (const uint8_t*) file, filelen);
		default:
			return 0;
	}
}

} // namespace ELF
} // namespace Sortix
