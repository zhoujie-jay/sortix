/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    print.cpp
    Provides the stubs for the printf family of functions.

*******************************************************************************/

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static size_t FileWriteCallback(void* user, const char* string, size_t stringlen)
{
	FILE* fp = (FILE*) user;
	return fwrite(string, 1, stringlen, fp);
}

extern "C" int vfprintf(FILE* fp, const char* /*restrict*/ format, va_list list)
{
	size_t result = vprintf_callback(FileWriteCallback, fp, format, list);
	if ( result == SIZE_MAX )
		return -1;
	return (int) result;
}

extern "C" int fprintf(FILE* fp, const char* /*restrict*/ format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vfprintf(fp, format, list);
	va_end(list);
	return result;
}

extern "C" int vprintf(const char* /*restrict*/ format, va_list list)
{
	return vfprintf(stdout, format, list);
}

extern "C" int printf(const char* /*restrict*/ format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vprintf(format, list);
	va_end(list);
	return result;
}
