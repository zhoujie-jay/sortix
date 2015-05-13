/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    assert.h
    Verify program assertion.

*******************************************************************************/

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
