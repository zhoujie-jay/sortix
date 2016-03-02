/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * string/stresep.c
 * Extract a token from a string while handling escape characters.
 */

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
