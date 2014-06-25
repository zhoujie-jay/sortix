/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    system-elf.h
    Executable and Linkable Format for the local system.

*******************************************************************************/

#ifndef INCLUDE_SYSTEM_ELF_H
#define INCLUDE_SYSTEM_ELF_H

#include <sys/cdefs.h>

#include <elf.h>

#include <__/wordsize.h>

__BEGIN_DECLS

#if __WORDSIZE == 32
typedef Elf32_Half Elf_Half;
typedef Elf32_Word Elf_Word;
typedef Elf32_Sword Elf_Sword;
typedef Elf32_Xword Elf_Xword;
typedef Elf32_Sxword Elf_Sxword;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Off Elf_Off;
typedef Elf32_Section Elf_Section;
typedef Elf32_Versym Elf_Versym;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Syminfo Elf_Syminfo;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Phdr Elf_Phdr;
typedef Elf32_Dyn Elf_Dyn;
typedef Elf32_Verdef Elf_Verdef;
typedef Elf32_Verdaux Elf_Verdaux;
typedef Elf32_Verneed Elf_Verneed;
typedef Elf32_Vernaux Elf_Vernaux;
typedef Elf32_auxv_t Elf_auxv_t;
typedef Elf32_Nhdr Elf_Nhdr;
typedef Elf32_Move Elf_Move;
typedef Elf32_Lib Elf_Lib;
#elif __WORDSIZE == 64
typedef Elf64_Half Elf_Half;
typedef Elf64_Word Elf_Word;
typedef Elf64_Sword Elf_Sword;
typedef Elf64_Xword Elf_Xword;
typedef Elf64_Sxword Elf_Sxword;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Off Elf_Off;
typedef Elf64_Section Elf_Section;
typedef Elf64_Versym Elf_Versym;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym Elf_Sym;
typedef Elf64_Syminfo Elf_Syminfo;
typedef Elf64_Rel Elf_Rel;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Dyn Elf_Dyn;
typedef Elf64_Verdef Elf_Verdef;
typedef Elf64_Verdaux Elf_Verdaux;
typedef Elf64_Verneed Elf_Verneed;
typedef Elf64_Vernaux Elf_Vernaux;
typedef Elf64_auxv_t Elf_auxv_t;
typedef Elf64_Nhdr Elf_Nhdr;
typedef Elf64_Move Elf_Move;
typedef Elf64_Lib Elf_Lib;
#else
#error "You need to typedef these to the types of your platform."
#endif

__END_DECLS

#endif
