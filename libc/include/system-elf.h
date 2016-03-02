/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * system-elf.h
 * Executable and Linkable Format for the local system.
 */

#ifndef INCLUDE_SYSTEM_ELF_H
#define INCLUDE_SYSTEM_ELF_H

#include <sys/cdefs.h>

#include <elf.h>

#include <__/wordsize.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
