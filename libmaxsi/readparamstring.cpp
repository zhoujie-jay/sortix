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

	readparamstring.cpp
	A kinda dirty hack to ease transition to the /dev/video API.

*******************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <readparamstring.h>

static char* Substring(const char* src, size_t offset, size_t length)
{
	size_t srclen = strlen(src);
	char* dest = new char[length + 1];
	if ( !dest ) { return NULL; }
	memcpy(dest, src + offset, length * sizeof(char));
	dest[length] = 0;
	return dest;
}

extern "C" bool ReadParamString(const char* str, ...)
{
	if ( strchr(str, '\n') ) { errno = EINVAL; }
	const char* keyname;
	va_list args;
	while ( *str )
	{
		size_t varlen = strcspn(str, ",");
		if ( !varlen ) { str++; continue; }
		size_t namelen = strcspn(str, "=");
		if ( !namelen ) { errno = EINVAL; goto cleanup; }
		if ( !str[namelen] ) { errno = EINVAL; goto cleanup; }
		if ( varlen < namelen ) { errno = EINVAL; goto cleanup; }
		size_t valuelen = varlen - 1 /*=*/ - namelen;
		char* name = Substring(str, 0, namelen);
		if ( !name ) { goto cleanup; }
		char* value = Substring(str, namelen+1, valuelen);
		if ( !value ) { delete[] name; goto cleanup; }
		va_start(args, str);
		while ( (keyname = va_arg(args, const char*)) )
		{
			char** nameptr = va_arg(args, char**);
			if ( strcmp(keyname, name) ) { continue; }
			*nameptr = value;
			break;
		}
		va_end(args);
		if ( !keyname ) { delete[] value; }
		delete[] name;
		str += varlen;
		str += strspn(str, ",");
	}
	return true;

cleanup:
	va_start(args, str);
	while ( (keyname = va_arg(args, const char*)) )
	{
		char** nameptr = va_arg(args, char**);
		delete[] *nameptr; *nameptr = NULL;
	}
	va_end(args);
	return false;
}
