/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    string/strsep.cpp
    Extract a token from a string.

*******************************************************************************/

#include <string.h>

extern "C" char* strsep(char** string_ptr, const char* delim)
{
	char* token = *string_ptr;
	if ( !token )
		return NULL;
	size_t token_length = strcspn(token, delim);
	if ( token[token_length] )
	{
		token[token_length++] = '\0';
		*string_ptr = token + token_length;
	}
	else
	{
		*string_ptr = (char*) NULL;
	}
	return token;
}
