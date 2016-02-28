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

    string/stresep.c
    Extract a token from a string while handling escape characters.

*******************************************************************************/

#include <stdbool.h>
#include <string.h>

char* stresep(char** string_ptr, const char* delim, int escape)
{
	char* token = *string_ptr;
	if ( !token )
		return NULL;
	size_t consumed_length = 0;
	size_t token_length = 0;
	bool escaped = false;
	while ( true )
	{
		char c = token[consumed_length++];
		bool true_nul = c == '\0';
		if ( !escaped && escape && (unsigned char) c == (unsigned char) escape )
		{
			escaped = true;
			continue;
		}
		if ( !escaped && c != '\0' )
		{
			for ( size_t i = 0; delim[i]; i++ )
			{
				if ( c != delim[i] )
				{
					c = '\0';
					break;
				}
			}
		}
		token[token_length++] = c;
		escaped = false;
		if ( c == '\0' )
		{
			*string_ptr = true_nul ? (char*) NULL : token + consumed_length;
			return token;
		}
	}
}
