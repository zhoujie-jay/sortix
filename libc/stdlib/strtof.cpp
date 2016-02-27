/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2016.

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
#define STRTOF_CTYPE_CHAR unsigned char
#define STRTOF_L(x) x
#define STRTOF_ISSPACE isspace
#define STRTOF_STRNCASECMP strncasecmp
#define STRTOF_POW powf
#endif

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

// TODO: This is an imperfect implementation that doesn't try to minimize loss
//       and explicitly handle overflow conditions. It behaves well enough on
//       simple common inputs to do for now.

#if !defined(__is_sortix_kernel)
static inline size_t nan_parameter(const STRTOF_CHAR* str)
{
	size_t index = 0;
	if ( str[index++] != STRTOF_L('(') )
		return 0;
	// TODO: What exactly does digit and nondigit mean here in POSIX?
	if ( !str[index] || str[index] == STRTOF_L(')') )
		return 0;
	while ( str[index] && str[index] != STRTOF_L(')') )
		index++;
	if ( str[index++] != STRTOF_L(')') )
		return 0;
	return index;
}

static inline bool is_hexadecimal_float(const STRTOF_CHAR* str)
{
	if ( *str++ != '0' )
		return false;
	if ( *str != 'x' && *str != 'X' )
		return false;
	str++;
	if ( *str == '.' )
		str++;
	return isxdigit((STRTOF_CTYPE_CHAR) *str);
}

static inline bool is_exponent(const STRTOF_CHAR* str,
                               STRTOF_CHAR elc,
                               STRTOF_CHAR euc)
{
	if ( *str != elc && *str != euc )
		return false;
	str++;
	if ( *str == '-' || *str == '+' )
		str++;
	return STRTOF_L('0') <= *str && *str <= STRTOF_L('9');
}

static inline bool parse_digit(const STRTOF_CHAR* str,
                               int base,
                               STRTOF_FLOAT* out)
{
	if ( STRTOF_L('0') <= *str && *str <= STRTOF_L('9') )
		return *out = *str - STRTOF_L('0') , true;
	if ( base == 16 )
	{
		if ( STRTOF_L('a') <= *str && *str <= STRTOF_L('f') )
			return *out = 10 + *str - STRTOF_L('a') , true;
		if ( STRTOF_L('A') <= *str && *str <= STRTOF_L('F') )
			return *out = 10 + *str - STRTOF_L('A') , true;
	}
	return false;
}

extern "C" STRTOF_FLOAT STRTOF(const STRTOF_CHAR* str, STRTOF_CHAR** nptr)
{
	if ( nptr )
		*nptr = (STRTOF_CHAR*) str;

	while ( *str && STRTOF_ISSPACE((STRTOF_CTYPE_CHAR) *str) )
		str++;

	if ( !STRTOF_STRNCASECMP(str, STRTOF_L("INF"), 3) )
	{
		str += 3;
		if ( !STRTOF_STRNCASECMP(str, STRTOF_L("INITY"), 5) )
			str += 5;
		if ( nptr )
			*nptr = (STRTOF_CHAR*) str;
		return INFINITY;
	}

	if ( !STRTOF_STRNCASECMP(str, STRTOF_L("NAN"), 3) )
	{
		str += 3;
		str += nan_parameter(str);
		if ( nptr )
			*nptr = (STRTOF_CHAR*) str;
		return NAN;
	}

	bool negative = *str == STRTOF_L('-');
	if ( *str == STRTOF_L('-') )
		str++;
	else if ( *str == STRTOF_L('+') )
		str++;

	int base = 10;
	STRTOF_CHAR elc = 'e';
	STRTOF_CHAR euc = 'E';

	if ( is_hexadecimal_float(str) )
	{
		str += 2;
		base = 16;
		elc = 'p';
		euc = 'P';
	}

	bool any_digits = false;
	STRTOF_FLOAT result = 0.0;
	STRTOF_FLOAT add;

	while ( parse_digit(str, base, &add) )
	{
		str++;
		result = base * result + add;
		any_digits = true;
	}

	if ( *str == STRTOF_L('.' ) )
	{
		str++;
		STRTOF_FLOAT magnitude = 1.0;
		while ( parse_digit(str, base, &add) )
		{
			str++;
			magnitude /= base;
			result += add * magnitude;
			any_digits = true;
		}
	}

	if ( !any_digits )
		return errno = EINVAL, 0;

	if ( is_exponent(str, elc, euc) )
	{
		str++;
		bool exponent_negative = *str == STRTOF_L('-');
		if ( *str == STRTOF_L('-') )
			str++;
		else if ( *str == STRTOF_L('+') )
			str++;
		STRTOF_FLOAT exponent = 0.0;
		while ( parse_digit(str, 10, &add) )
		{
			str++;
			exponent = 10.0 * exponent + add;
			any_digits = true;
		}
		if ( exponent_negative )
			exponent = -exponent;
		result *= STRTOF_POW(base == 16 ? 2 : base, exponent);
	}

	if ( negative )
		result = -result;

	if ( nptr )
		*nptr = (STRTOF_CHAR*) str;
	return result;
}
#endif
