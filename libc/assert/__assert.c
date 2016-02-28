/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2015.

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

    assert/__assert.c
    Reports the occurence of an assertion failure.

*******************************************************************************/

#include <assert.h>
#include <scram.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

void __assert(const char* filename,
              unsigned long line,
              const char* function_name,
              const char* expression)
{
#ifdef __is_sortix_libk
	libk_assert(filename, line, function_name, expression);
#else
	struct scram_assert info;
	info.filename = filename;
	info.line = line;
	info.function = function_name;
	info.expression = expression;
	scram(SCRAM_ASSERT, &info);
#endif
}
