/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    libk.h
    Standalone C library hooks.

*******************************************************************************/

#ifndef INCLUDE_LIBK_H
#define INCLUDE_LIBK_H

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((noreturn))
void libk_assert(const char*, unsigned long, const char*, const char*);
size_t libk_getpagesize(void);
void* libk_heap_expand(size_t*);
void libk_heap_lock(void);
void libk_heap_unlock(void);
__attribute__((noreturn))
void libk_stack_chk_fail(void);
__attribute__((noreturn))
void libk_abort(void);
void libk_random_lock(void);
void libk_random_unlock(void);
bool libk_hasentropy(void);
void libk_getentropy(void*, size_t);
__attribute__((noreturn))
void libk_ubsan_abort(const char*, const char*, uint32_t, uint32_t);
void* libk_mmap(size_t, int);
void libk_mprotect(void*, size_t, int);
void libk_munmap(void*, size_t);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
