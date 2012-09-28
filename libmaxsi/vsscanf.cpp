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

	vsscanf.cpp
	Input format conversion.

*******************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern "C" int vsscanf(const char* str, const char* format, va_list ap)
{
	const char* filename = "/ugly-vsscanf-hack";
	FILE* fp = fopen(filename, "w+");
	if ( !fp )
		return -1;
	int ret = -1;
	size_t len = strlen(str);
	if ( fwrite(str, sizeof(char), len, fp) == len )
		if ( fseek(fp, 0, SEEK_SET) == 0 )
			ret = vfscanf(fp, format, ap);
	int savederrno = errno;
	fclose(fp);
	unlink(filename);
	errno = savederrno;
	return ret;
}
