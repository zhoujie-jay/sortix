/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of Sortix libmount.

    Sortix libmount is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libmount is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libmount. If not, see <http://www.gnu.org/licenses/>.

    uuid.c
    Universally unique identifier.

*******************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <mount/uuid.h>

static bool is_hex_digit(char c)
{
	return ('0' <= c && c <= '9') ||
	       ('a' <= c && c <= 'f') ||
	       ('A' <= c && c <= 'F');
}

static char base(unsigned char bit4)
{
	return "0123456789abcdef"[bit4];
}

static unsigned char debase(char c)
{
	if ( '0' <= c && c <= '9' )
		return (unsigned char) (c - '0');
	if ( 'a' <= c && c <= 'f' )
		return (unsigned char) (c - 'a' + 10);
	if ( 'A' <= c && c <= 'F' )
		return (unsigned char) (c - 'A' + 10);
	return 0;
}

bool uuid_validate(const char* uuid)
{
	// Format: 01234567-0123-0123-0123-0123456789AB
	if ( strlen(uuid) != 36 )
		return false;
	for ( size_t i = 0; i < 36; i++ )
	{
		if ( i == 8 || i == 13 || i == 18 || i == 23 )
		{
			if ( uuid[i] != '-' )
				return false;
		}
		else
		{
			if ( !is_hex_digit(uuid[i]) )
				return false;
		}
	}
	return true;
}

void uuid_from_string(unsigned char uuid[16], const char* string)
{
	size_t output_index = 0;
	size_t i = 0;
	while ( i < 36 )
	{
		assert(string[i + 0] != '\0');
		if ( i == 8 || i == 13 || i == 18 || i == 23 )
		{
			i++;
			continue;
		}
		uuid[output_index++] = debase(string[i + 0]) << 4 |
		                       debase(string[i + 1]) << 0;
		i += 2;
	}
}

void uuid_to_string(const unsigned char uuid[16], char* string)
{
	for ( size_t i = 0; i < 4; i++ )
	{
		*string++ = base((uuid[i] >> 4) & 0xF);
		*string++ = base((uuid[i] >> 0) & 0xF);
	}
	*string++ = '-';
	for ( size_t i = 4; i < 6; i++ )
	{
		*string++ = base((uuid[i] >> 4) & 0xF);
		*string++ = base((uuid[i] >> 0) & 0xF);
	}
	*string++ = '-';
	for ( size_t i = 6; i < 8; i++ )
	{
		*string++ = base((uuid[i] >> 4) & 0xF);
		*string++ = base((uuid[i] >> 0) & 0xF);
	}
	*string++ = '-';
	for ( size_t i = 8; i < 10; i++ )
	{
		*string++ = base((uuid[i] >> 4) & 0xF);
		*string++ = base((uuid[i] >> 0) & 0xF);
	}
	*string++ = '-';
	for ( size_t i = 10; i < 16; i++ )
	{
		*string++ = base((uuid[i] >> 4) & 0xF);
		*string++ = base((uuid[i] >> 0) & 0xF);
	}
	*string = '\0';
}
