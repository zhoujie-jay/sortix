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

    assert/__assert.cpp
    Reports the occurence of an assertion failure.

*******************************************************************************/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__is_sortix_kernel)
#include <sortix/kernel/decl.h>
#include <sortix/kernel/panic.h>
#endif

extern "C"
void __assert(const char* filename,
              unsigned long line,
              const char* function_name,
              const char* expression)
{
#if defined(__is_sortix_kernel)
	Sortix::PanicF("Assertion failure: %s:%lu: %s: %s", filename, line,
	               function_name, expression);
#else
	fprintf(stderr, "Assertion failure: %s:%lu: %s: %s\n", filename, line,
	        function_name, expression);
	abort();
#endif
}
