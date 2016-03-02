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
 * sortix/kernel/elf.h
 * Executable and Linkable Format.
 */

#ifndef INCLUDE_SORTIX_KERNEL_ELF_H
#define INCLUDE_SORTIX_KERNEL_ELF_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {
namespace ELF {

struct Auxiliary
{
	size_t tls_file_offset;
	size_t tls_file_size;
	size_t tls_mem_size;
	size_t tls_mem_align;
	size_t uthread_size;
	size_t uthread_align;
};

// Reads the elf file into the current address space and returns the entry
// address of the program, or 0 upon failure.
uintptr_t Load(const void* file, size_t filelen, Auxiliary* aux);

} // namespace ELF
} // namespace Sortix

#endif
