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

    assert/_assert.cpp
    Reports the occurence of an assertion failure.

*******************************************************************************/

#include <assert.h>
#include <stdint.h>
#if !defined(SORTIX_KERNEL)
#include <stdio.h>
#include <stdlib.h>
#endif
#if defined(SORTIX_KERNEL)
#include <sortix/kernel/decl.h>
#include <sortix/kernel/panic.h>
#endif

void _assert(const char* filename, unsigned int line, const char* functionname,
             const char* expression)
{
#if !defined(SORTIX_KERNEL)
	fprintf(stderr, "Assertion failure: %s:%u: %s: %s\n", filename, line,
	        functionname, expression);
	abort();
#else
	Sortix::PanicF("Assertion failure: %s:%u: %s: %s\n", filename, line,
	               functionname, expression);
#endif
}
