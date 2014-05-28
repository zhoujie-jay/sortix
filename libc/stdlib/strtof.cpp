/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    stdlib/strtof.cpp
    Converts floating numbers represented as strings to binary representation.

*******************************************************************************/

#ifndef STRTOF
#define STRTOF_FLOAT float
#define STRTOF strtof
#define STRTOF_CHAR char
#define STRTOF_L(x) x
#endif

#include <stdlib.h>
#include <wchar.h>

// TODO: This horribly hacky code is taken from sdlquake's common.c, which is
//       licensed under the GPL. Since most Sortix software is GPL-compatible
//       this will do for now. It's a temporary measure until I get around to
//       writing a real strtod function - most of them are true horrors.

#if !defined(__is_sortix_kernel)
extern "C" STRTOF_FLOAT STRTOF(const STRTOF_CHAR* str, STRTOF_CHAR** nptr)
{
	int sign = *str == STRTOF_L('-') ? (str++, -1) : 1;
	STRTOF_FLOAT val = 0.0;

	if ( false )
	{
	out:
		if ( nptr )
			*nptr = (STRTOF_CHAR*) str;
		return val * sign;
	}

	if ( str[0] == STRTOF_L('0') &&
	     (str[1] == STRTOF_L('x') || str[1] == STRTOF_L('X')) )
	{
		str += 2;
		while ( true )
		{
			STRTOF_CHAR c = *str++;
			if ( STRTOF_L('0') <= c && c <= STRTOF_L('9') )
				val = val * 16 + c - STRTOF_L('0');
			else if ( STRTOF_L('a') <= c && c <= STRTOF_L('f') )
				val = val * 16 + c - STRTOF_L('a') + 10;
			else if ( STRTOF_L('A') <= c && c <= STRTOF_L('F') )
				val = val * 16 + c - STRTOF_L('A') + 10;
			else
				goto out;
		}
	}

	int decimal = -1;
	int total = 0;
	while ( true )
	{
		STRTOF_CHAR c = *str++;
		if ( c == STRTOF_L('.') )
		{
			decimal = total;
			continue;
		}
		if ( c < STRTOF_L('0') || c > STRTOF_L('9') )
			break;
		val = val * 10 + c - STRTOF_L('0');
		total++;
	}

	if ( decimal == -1 )
		goto out;

	while ( decimal < total )
	{
		val /= 10;
		total--;
	}

	goto out;
}
#endif
