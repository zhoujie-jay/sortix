/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	print.cpp
	Provides the stubs for the printf family of functions.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/format.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

namespace Maxsi {

static size_t FileWriteCallback(void* user, const char* string, size_t stringlen)
{
	FILE* fp = (FILE*) user;
	return fwrite(string, 1, stringlen, fp);
}

extern "C" int vfprintf(FILE* fp, const char* /*restrict*/ format, va_list list)
{
	size_t result = Maxsi::Format::Virtual(FileWriteCallback, fp, format, list);
	return (int) result;
}

extern "C" int fprintf(FILE* fp, const char* /*restrict*/ format, ...)
{
	va_list list;
	va_start(list, format);
	size_t result = vfprintf(fp, format, list);
	va_end(list);
	return (int) result;
}

extern "C" int vprintf(const char* /*restrict*/ format, va_list list)
{
	size_t result = vfprintf(stdout, format, list);
	return (int) result;
}

extern "C" int printf(const char* /*restrict*/ format, ...)
{
	va_list list;
	va_start(list, format);
	size_t result = vprintf(format, list);
	va_end(list);
	return (int) result;
}

typedef struct vsnprintf_struct
{
	char* str;
	size_t size;
	size_t produced;
	size_t written;
} vsnprintf_t;

static size_t StringPrintCallback(void* user, const char* string, size_t stringlen)
{
	vsnprintf_t* info = (vsnprintf_t*) user;
	if ( info->produced < info->size )
	{
		size_t available = info->size - info->produced;
		size_t possible = (stringlen < available) ? stringlen : available;
		memcpy(info->str + info->produced, string, possible);
		info->written += possible;
	}
	info->produced += stringlen;
	return stringlen;
}

extern "C" int vsnprintf(char* restrict str, size_t size, const char* restrict format, va_list list)
{
	vsnprintf_t info;
	info.str = str;
	info.size = (size) ? size-1 : 0;
	info.produced = 0;
	info.written = 0;
	Maxsi::Format::Virtual(StringPrintCallback, &info, format, list);
	if ( size ) { info.str[info.written] = '\0'; }
	return (int) info.produced;
}

extern "C" int snprintf(char* restrict str, size_t size, const char* restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vsnprintf(str, size, format, list);
	va_end(list);
	return result;
}

extern "C" int vsprintf(char* restrict str, const char* restrict format, va_list list)
{
	return vsnprintf(str, SIZE_MAX, format, list);
}

extern "C" int sprintf(char* restrict str, const char* restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vsprintf(str, format, list);
	va_end(list);
	return result;
}

} // namespace Maxsi
