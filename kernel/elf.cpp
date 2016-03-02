/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * elf.cpp
 * Load a program in the Executable and Linkable Format into this process.
 */

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

	return header->e_entry;
}

} // namespace ELF
} // namespace Sortix
