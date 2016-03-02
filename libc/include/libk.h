/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * libk.h
 * Standalone C library hooks.
 */

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
