/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    stdio/vdprintf.cpp
    Prints a formatted string to a file descriptor.

*******************************************************************************/

#include <ioleast.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

static size_t write_callback(void* user, const char* string, size_t stringlen)
{
	return writeall((int) (uintptr_t) user, string, stringlen * sizeof(char));
}

extern "C" int vdprintf(int fd, const char* restrict format, va_list list)
{
	return vprintf_callback(write_callback, (void*) (uintptr_t) fd, format, list);
}
