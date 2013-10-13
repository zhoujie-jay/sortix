/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

#ifndef _ASSERT_H
#define _ASSERT_H 1

#include <sys/cdefs.h>

/* stdlib.h is not needed, but GCC fixincludes thinks it is, so fool it. */
#if 0
#include <stdlib.h>
#endif

__BEGIN_DECLS

/* The actual implementation of assert. */
void _assert(const char* filename, unsigned int line, const char* functionname,
             const char* expression) __attribute__ ((noreturn));

__END_DECLS

#endif

/* Rid ourselves of any previous declaration of assert. */
#ifdef assert
#undef assert
#endif

/* Redefine the assert macro on each <assert.h> inclusion. */
#ifdef NDEBUG

#define assert(ignore) ((void) 0)

#else /* !NDEBUG */

/* Use __builtin_expect to tell the compiler that we don't expect a failure to
   happen and thus it can do better branch prediction. Naturally we don't
   optimize for the case where the program is about to abort(). */
#define assert(invariant) \
	if ( __builtin_expect(!(invariant), 0) ) \
	{ \
		_assert(__FILE__, __LINE__, __PRETTY_FUNCTION__, #invariant); \
	}

#endif /* !NDEBUG */
