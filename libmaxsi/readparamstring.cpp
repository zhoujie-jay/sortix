/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	readparamstring.cpp
	A kinda dirty hack to ease transition to the /dev/video API.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include <readparamstring.h>

using namespace Maxsi;

extern "C" bool ReadParamString(const char* str, ...)
{
	if ( String::Seek(str, '\n') ) { Error::Set(EINVAL); }
	const char* keyname;
	va_list args;
	while ( *str )
	{
		size_t varlen = String::Reject(str, ",");
		if ( !varlen ) { str++; continue; }
		size_t namelen = String::Reject(str, "=");
		if ( !namelen ) { Error::Set(EINVAL); goto cleanup; }
		if ( !str[namelen] ) { Error::Set(EINVAL); goto cleanup; }
		if ( varlen < namelen ) { Error::Set(EINVAL); goto cleanup; }
		size_t valuelen = varlen - 1 /*=*/ - namelen;
		char* name = String::Substring(str, 0, namelen);
		if ( !name ) { goto cleanup; }
		char* value = String::Substring(str, namelen+1, valuelen);
		if ( !value ) { delete[] name; goto cleanup; }
		va_start(args, str);
		while ( (keyname = va_arg(args, const char*)) )
		{
			char** nameptr = va_arg(args, char**);
			if ( String::Compare(keyname, name) ) { continue; }
			*nameptr = value;
			break;
		}
		va_end(args);
		if ( !keyname ) { delete[] value; }
		delete[] name;
		str += varlen;
		str += String::Accept(str, ",");
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
