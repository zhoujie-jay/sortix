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
    Load a program in the Executable and Linkable Format into this process.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <elf.h>
#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <system-elf.h>

#include <__/wordsize.h>

#include <sortix/mman.h>

#include <sortix/kernel/elf.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>
#include <sortix/kernel/symbol.h>

namespace Sortix {
namespace ELF {

static bool is_power_of_two(uintptr_t value)
{
	for ( uintptr_t i = 0; i < sizeof(uintptr_t) * 8; i++ )
		if ( (uintptr_t) 1 << i == value )
			return true;
	return false;
}

uintptr_t Load(const void* file_ptr, size_t file_size, Auxiliary* aux)
{
	memset(aux, 0, sizeof(*aux));

	Process* process = CurrentProcess();

	uintptr_t userspace_addr;
	size_t userspace_size;
	Memory::GetUserVirtualArea(&userspace_addr, &userspace_size);
	uintptr_t userspace_end = userspace_addr + userspace_size;

	const unsigned char* file = (const unsigned char*) file_ptr;

	if ( file_size < EI_NIDENT )
		return errno = ENOEXEC, 0;

	if ( memcmp(file, ELFMAG, SELFMAG) != 0 )
		return errno = ENOEXEC, 0;

#if __WORDSIZE == 32
	if ( file[EI_CLASS] != ELFCLASS32 )
		return errno = EINVAL, 0;
#elif __WORDSIZE == 64
	if ( file[EI_CLASS] != ELFCLASS64 )
		return errno = EINVAL, 0;
#else
#error "You need to add support for your elf class."
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
	if ( file[EI_DATA] != ELFDATA2LSB )
		return errno = EINVAL, 0;
#elif BYTE_ORDER == BIG_ENDIAN
	if ( file[EI_DATA] != ELFDATA2MSB )
		return errno = EINVAL, 0;
#else
#error "You need to add support for your endian."
#endif

	if ( file[EI_VERSION] != EV_CURRENT )
		return errno = EINVAL, 0;

	if ( file[EI_OSABI] != ELFOSABI_SORTIX )
		return errno = EINVAL, 0;

	if ( file[EI_ABIVERSION] != 0 )
		return errno = EINVAL, 0;

	if ( file_size < sizeof(Elf_Ehdr) )
		return errno = EINVAL, 0;
	if ( (uintptr_t) file & (alignof(Elf_Ehdr) - 1) )
		return errno = EINVAL, 0;
	const Elf_Ehdr* header = (const Elf_Ehdr*) file;

	if ( header->e_ehsize < sizeof(Elf_Ehdr) )
		return errno = EINVAL, 0;

	if ( file_size < header->e_ehsize )
		return errno = EINVAL, 0;

#if defined(__i386__)
	if ( header->e_machine != EM_386 )
		return errno = EINVAL, 0;
#elif defined(__x86_64__)
	if ( header->e_machine != EM_X86_64 )
		return errno = EINVAL, 0;
#else
#error "Please recognize your processor in e_machine."
#endif

	if ( header->e_type != ET_EXEC )
		return errno = EINVAL, 0;

	if ( header->e_entry == 0 )
		return errno = EINVAL, 0;

	if ( file_size < header->e_phoff )
		return errno = EINVAL, 0;

	if ( file_size < header->e_shoff )
		return errno = EINVAL, 0;

	if ( header->e_phentsize < sizeof(Elf_Phdr) )
		return errno = EINVAL, 0;

	if ( header->e_shentsize < sizeof(Elf_Shdr) )
		return errno = EINVAL, 0;

	process->ResetForExecute();

	if ( header->e_phnum == (Elf_Half) -1 )
		return errno = EINVAL, 0;
	if ( header->e_shnum == (Elf_Half) -1 )
		return errno = EINVAL, 0;

	for ( Elf32_Half i = 0; i < header->e_phnum; i++ )
	{
		size_t max_phs = (file_size - header->e_phoff) / header->e_phentsize;
		if ( max_phs <= i )
			return errno = EINVAL, 0;
		size_t pheader_offset = header->e_phoff + i * header->e_phentsize;
		if ( (uintptr_t) (file + pheader_offset) & (alignof(Elf_Phdr) - 1) )
			return errno = EINVAL, 0;
		Elf_Phdr* pheader = (Elf_Phdr*) (file + pheader_offset);

		switch ( pheader->p_type )
		{
		case PT_TLS: break;
		case PT_NOTE: break;
		case PT_LOAD: break;
		default: continue;
		};

		if ( !is_power_of_two(pheader->p_align) )
			return errno = EINVAL, 0;
		if ( file_size < pheader->p_offset  )
			return errno = EINVAL, 0;
		if ( file_size - pheader->p_offset < pheader->p_filesz )
			return errno = EINVAL, 0;

		if ( pheader->p_type == PT_TLS )
		{
			if ( pheader->p_memsz < pheader->p_filesz )
				return errno = EINVAL, 0;

			aux->tls_file_offset = pheader->p_offset;
			aux->tls_file_size = pheader->p_filesz;
			aux->tls_mem_size = pheader->p_memsz;
			aux->tls_mem_align = pheader->p_align;
			continue;
		}

		if ( pheader->p_type == PT_NOTE )
		{
			size_t notes_offset = 0;
			while ( notes_offset < pheader->p_filesz )
			{
				size_t available = pheader->p_filesz - notes_offset;
				size_t note_header_size = 3 * sizeof(uint32_t);
				if ( available < note_header_size )
					return errno = EINVAL, 0;
				available -= note_header_size;
				size_t file_offset = pheader->p_offset + notes_offset;
				if ( ((uintptr_t) file + file_offset) & (alignof(uint32_t) - 1) )
					return errno = EINVAL, 0;

				const unsigned char* note = file + file_offset;
				uint32_t* note_header = (uint32_t*) note;
				uint32_t namesz = note_header[0];
				uint32_t descsz = note_header[1];
				uint32_t type = note_header[2];
				uint32_t namesz_aligned = -(-namesz & ~(sizeof(uint32_t) - 1));
				uint32_t descsz_aligned = -(-descsz & ~(sizeof(uint32_t) - 1));
				if ( available < namesz_aligned )
					return errno = EINVAL, 0;
				available -= namesz_aligned;
				if ( available < descsz_aligned )
					return errno = EINVAL, 0;
				available -= descsz_aligned;
				(void) available;
				notes_offset += note_header_size + namesz_aligned + descsz_aligned;

				const char* name = (const char*) (note + note_header_size);
				if ( strnlen(name, namesz_aligned) == namesz_aligned )
					return errno = EINVAL, 0;
				const unsigned char* desc = note + note_header_size + namesz_aligned;
				const uint32_t* desc_32bits = (const uint32_t*) desc;

				if ( strcmp(name, ELF_NOTE_SORTIX) == 0 )
				{
					if ( type == ELF_NOTE_SORTIX_UTHREAD_SIZE )
					{
						if ( descsz_aligned != 2 * sizeof(size_t) )
							return errno = EINVAL, 0;
#if __WORDSIZE == 32
						aux->uthread_size = desc_32bits[0];
						aux->uthread_align = desc_32bits[1];
#elif __WORDSIZE == 64 && BYTE_ORDER == LITTLE_ENDIAN
						aux->uthread_size = (uint64_t) desc_32bits[0] << 0 |
						                    (uint64_t) desc_32bits[1] << 32;
						aux->uthread_align = (uint64_t) desc_32bits[2] << 0 |
						                     (uint64_t) desc_32bits[3] << 32;
#elif __WORDSIZE == 64 && BYTE_ORDER == BIG_ENDIAN
						aux->uthread_size = (uint64_t) desc_32bits[1] << 0 |
						                    (uint64_t) desc_32bits[0] << 32;
						aux->uthread_align = (uint64_t) desc_32bits[3] << 0 |
						                     (uint64_t) desc_32bits[2] << 32;
#else
#error "You need to correctly read the uthread note"
#endif
						if ( !is_power_of_two(aux->uthread_align) )
							return errno = EINVAL, 0;
					}
				}
			}
			continue;
		}

		if ( pheader->p_type == PT_LOAD )
		{
			if ( pheader->p_memsz < pheader->p_filesz )
				return errno = EINVAL, 0;
			if ( pheader->p_filesz &&
			     pheader->p_vaddr % pheader->p_align !=
			     pheader->p_offset % pheader->p_align )
				return errno = EINVAL, 0;
			int kprot = PROT_KWRITE | PROT_FORK;
			int prot = PROT_FORK;
			if ( pheader->p_flags & PF_X )
				prot |= PROT_EXEC;
			if ( pheader->p_flags & PF_R )
				prot |= PROT_READ | PROT_KREAD;
			if ( pheader->p_flags & PF_W )
				prot |= PROT_WRITE | PROT_KWRITE;

			if ( pheader->p_vaddr < userspace_addr )
				return errno = EINVAL, 0;
			if ( userspace_end < pheader->p_vaddr )
				return errno = EINVAL, 0;
			if ( userspace_end - pheader->p_vaddr < pheader->p_memsz )
				return errno = EINVAL, 0;

			uintptr_t map_start = Page::AlignDown(pheader->p_vaddr);
			uintptr_t map_end = Page::AlignUp(pheader->p_vaddr + pheader->p_memsz);
			size_t map_size = map_end - map_start;

			struct segment segment;
			segment.addr =  map_start;
			segment.size = map_size;
			segment.prot = kprot;

			assert(IsUserspaceSegment(&segment));

			kthread_mutex_lock(&process->segment_write_lock);
			kthread_mutex_lock(&process->segment_lock);

			if ( IsSegmentOverlapping(process, &segment) )
			{
				kthread_mutex_unlock(&process->segment_lock);
				kthread_mutex_unlock(&process->segment_write_lock);
				return errno = EINVAL, 0;
			}

			if ( !Memory::MapRange(segment.addr, segment.size, kprot, PAGE_USAGE_USER_SPACE) )
			{
				kthread_mutex_unlock(&process->segment_lock);
				kthread_mutex_unlock(&process->segment_write_lock);
				return errno = EINVAL, 0;
			}

			if ( !AddSegment(process, &segment) )
			{
				Memory::UnmapRange(segment.addr, segment.size, PAGE_USAGE_USER_SPACE);
				kthread_mutex_unlock(&process->segment_lock);
				kthread_mutex_unlock(&process->segment_write_lock);
				return errno = EINVAL, 0;
			}

			memset((void*) segment.addr, 0, segment.size);
			memcpy((void*) pheader->p_vaddr, file + pheader->p_offset, pheader->p_filesz);
			Memory::ProtectMemory(CurrentProcess(), segment.addr, segment.size, prot);

			kthread_mutex_unlock(&process->segment_lock);
			kthread_mutex_unlock(&process->segment_write_lock);
		}
	}

	// SECURITY: TODO: Insecure.
	Elf_Shdr* section_string_table_section = (Elf_Shdr*) (file + header->e_shoff + header->e_shstrndx * header->e_shentsize);
	const char* section_string_table = (const char*) (file + section_string_table_section->sh_offset);
	//size_t section_string_table_size = section_string_table_section->sh_size;

	const char* string_table = NULL;
	size_t string_table_size = 0;

	for ( Elf32_Half i = 0; i < header->e_shnum; i++ )
	{
		size_t max_shs = (file_size - header->e_shoff) / header->e_shentsize;
		if ( max_shs <= i )
			return errno = EINVAL, 0;
		size_t sheader_offset = header->e_shoff + i * header->e_shentsize;
		if ( (uintptr_t) (file + sheader_offset) & (alignof(Elf_Shdr) - 1) )
			return errno = EINVAL, 0;
		Elf_Shdr* sheader = (Elf_Shdr*) (file + sheader_offset);

		// SECURITY: TODO: Insecure.
		const char* section_name = section_string_table + sheader->sh_name;

		if ( !strcmp(section_name, ".strtab") )
		{
			// SECURITY: TODO: Insecure.
			string_table = (const char*) (file + sheader->sh_offset);
			string_table_size = sheader->sh_size;
		}
	}

	for ( Elf32_Half i = 0; i < header->e_shnum; i++ )
	{
		size_t sheader_offset = header->e_shoff + i * header->e_shentsize;
		Elf_Shdr* sheader = (Elf_Shdr*) (file + sheader_offset);

		// SECURITY: TODO: Insecure.
		const char* section_name = section_string_table + sheader->sh_name;

		if ( !strcmp(section_name, ".symtab") )
		{
			if ( process->symbol_table )
				continue;

			// SECURITY: TODO: Insecure.
			Elf_Sym* symbols = (Elf_Sym*) (file + sheader->sh_offset);
			size_t symbols_length = sheader->sh_size / sizeof(Elf_Sym);

			if ( symbols_length == 0 )
				continue;

			symbols++;
			symbols_length--;

			char* string_table_copy = new char[string_table_size];
			if ( !string_table_copy )
				continue;
			memcpy(string_table_copy, string_table, string_table_size);

			Symbol* symbols_copy = new Symbol[symbols_length];
			if ( !symbols_copy )
			{
				delete[] string_table_copy;
				continue;
			}

			for ( size_t i = 0; i < symbols_length; i++ )
			{
				memset(&symbols_copy[i], 0, sizeof(symbols_copy[i]));
				symbols_copy[i].address = symbols[i].st_value;
				symbols_copy[i].size = symbols[i].st_size;
				// SECURITY: TODO: Insecure.
				symbols_copy[i].name = string_table_copy + symbols[i].st_name;
			}

			process->string_table = string_table_copy;
			process->string_table_length = string_table_size;
			process->symbol_table = symbols_copy;
			process->symbol_table_length = symbols_length;
		}
	}

	return header->e_entry;
}

} // namespace ELF
} // namespace Sortix
