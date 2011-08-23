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

#ifndef SORTIX_ELF_H
#define SORTIX_ELF_H

namespace Sortix
{
	namespace ELF
	{
		struct Header
		{
			unsigned char magic[4];
			unsigned char fileclass;
			unsigned char dataencoding;
			unsigned char version;
			unsigned char padding[9];
		};

		const unsigned char CLASSNONE = 0;
		const unsigned char CLASS32 = 1;
		const unsigned char CLASS64 = 2;
		const unsigned char DATA2LSB = 1;
		const unsigned char DATA2MSB = 2;
		const unsigned char CURRENTVERSION = 1;

		struct Header32 : public Header
		{
			uint16_t type;
			uint16_t machine;
			uint32_t version;
			uint32_t entry;
			uint32_t programheaderoffset;
			uint32_t sectionheaderoffset;
			uint32_t flags;
			uint16_t elfheadersize;
			uint16_t programheaderentrysize;
			uint16_t numprogramheaderentries;
			uint16_t sectionheaderentrysize;
			uint16_t numsectionheaderentries;
			uint16_t sectionheaderstringindex;
		};

		struct SectionHeader32
		{
			uint32_t name;
			uint32_t type;
			uint32_t flags;
			uint32_t addr;
			uint32_t offset;
			uint32_t size;
			uint32_t link;
			uint32_t info;
			uint32_t addralign;
			uint32_t entsize;
		};

		const uint32_t SHT_NULL = 0;
		const uint32_t SHT_PROGBITS = 1;
		const uint32_t SHT_SYMTAB = 2;
		const uint32_t SHT_STRTAB = 3;
		const uint32_t SHT_RELA = 4;
		const uint32_t SHT_HASH = 5;
		const uint32_t SHT_DYNAMIC = 6;
		const uint32_t SHT_NOTE = 7;
		const uint32_t SHT_NOBITS = 8;
		const uint32_t SHT_REL = 9;
		const uint32_t SHT_SHLIB = 10;
		const uint32_t SHT_DYNSYM = 11;
		const uint32_t SHT_LOPROC = 0x70000000;
		const uint32_t SHT_HIPROC = 0x7fffffff;
		const uint32_t SHT_LOUSER = 0x80000000;
		const uint32_t SHT_HIUSER = 0xffffffff;

		struct ProgramHeader32
		{
			uint32_t type;
			uint32_t offset;
			uint32_t virtualaddr;
			uint32_t physicaladdr;
			uint32_t filesize;
			uint32_t memorysize;
			uint32_t flags;
			uint32_t align;
		};

		const uint32_t PT_NULL = 0;
		const uint32_t PT_LOAD = 1;
		const uint32_t PT_DYNAMIC = 2;
		const uint32_t PT_INTERP = 3;
		const uint32_t PT_NOTE = 4;
		const uint32_t PT_SHLIB = 5;
		const uint32_t PT_PHDR = 6;
		const uint32_t PT_LOPROC = 0x70000000;
		const uint32_t PT_HIPROC = 0x7FFFFFFF;

		// Reads the elf file into the current address space and returns the
		// entry address of the program, or 0 upon failure.
		addr_t Construct(const void* file, size_t filelen);
	}
}

#endif
