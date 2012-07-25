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

	scan.cpp
	The scanf family of functions.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

namespace Maxsi {


// TODO: This is an ugly hack to help build binutils.
#warning Ugly sscanf hack to help build binutils
extern "C" int sscanf(const char* s, const char* format, ...)
{
	if ( strcmp(format, "%x") != 0 )
	{
		fprintf(stderr, "sscanf hack doesn't implement: '%s'\n", format);
		abort();
	}

	va_list list;
	va_start(list, format);
	unsigned* dec = va_arg(list, unsigned*);
	*dec = strtol(s, NULL, 16);
	return strlen(s);
}

extern "C" int fscanf(FILE* /*fp*/, const char* /*format*/, ...)
{
	fprintf(stderr, "fscanf(3) is not implemented\n");
	abort();
}

} // namespace Maxsi
