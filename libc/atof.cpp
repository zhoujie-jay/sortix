/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    atof.cpp
    Converts floats represented as strings to binary representation.

*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

// TODO: This horribly hacky code is taken from sdlquake's common.c, which is
//       licensed under the GPL. Since most Sortix software is GPL-compatible
//       this will do for now. It's a temporary measure until I get around to
//       writing a real strtod function - most of them are true horrors.

extern "C" double atof(const char* str)
{
	int sign = *str == '-' ? (str++, -1) : 1;
	double val = 0.0;

	if ( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while ( true )
		{
			char c = *str++;
			if ( '0' <= c && c <= '9' )
				val = val * 16 + c - '0';
			else if ( 'a' <= c && c <= 'f' )
				val = val * 16 + c - 'a' + 10;
			else if ( 'A' <= c && c <= 'F' )
				val = val * 16 + c - 'A' + 10;
			else
				return val * sign;
		}
	}

	int decimal = -1;
	int total = 0;
	while ( true )
	{
		char c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if ( c < '0' || c > '9' )
			break;
		val = val * 10 + c - '0';
		total++;
	}

	if ( decimal == -1 )
		return val * sign;

	while ( decimal < total )
	{
		val /= 10;
		total--;
	}

	return val * sign;
}
