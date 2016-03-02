/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
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
 * assert.h
 * Verify program assertion.
 */

#ifndef INCLUDE_ASSERT_H
#define INCLUDE_ASSERT_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Determine how the value should be cast to void. */
#if defined __cplusplus
#define __ASSERT_VOID_CAST(x) static_cast<void>(x)
#else
#define __ASSERT_VOID_CAST(x) (void) x
#endif

/* The C11 static_assert macro expands to the _Static_assert keyword. */
#if defined(__STDC_VERSION__) && 201112L <= __STDC_VERSION__
#define static_assert _Static_assert
#endif

/* The actual implementation of assert. */
__attribute__((noreturn))
void __assert(const char*, unsigned long, const char*, const char*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/* Rid ourselves of any previous declaration of assert. */
#ifdef assert
#undef assert
#endif

/* If not debugging, we'll declare a no-operation assert macro. */
#if defined(NDEBUG)
#define assert(ignore) (__ASSERT_VOID_CAST(0))
#endif

/* Otherwise, declare the normal assert macro. */
#if !defined(NDEBUG)
#define assert(invariant) \
  ((invariant) \
   ? __ASSERT_VOID_CAST(0) \
   : __assert(__FILE__, __LINE__, __PRETTY_FUNCTION__, #invariant))
#endif
