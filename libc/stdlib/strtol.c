/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013, 2014, 2015.

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

    stdlib/strtol.c
    Converts integers represented as strings to binary representation.

*******************************************************************************/

#ifndef STRTOL
#define STRTOL strtol
#define STRTOL_CHAR char
#define STRTOL_UCHAR unsigned char
#define STRTOL_L(x) x
#define STRTOL_ISSPACE isspace
#define STRTOL_INT long
#define STRTOL_UNSIGNED_INT unsigned long
#define STRTOL_INT_MIN LONG_MIN
#define STRTOL_INT_MAX LONG_MAX
#define STRTOL_INT_IS_UNSIGNED false
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// Convert a character into a digit.
static int debase(STRTOL_CHAR c)
{
	if ( STRTOL_L('0') <= c && c <= STRTOL_L('9') )
		return c - STRTOL_L('0');
	if ( STRTOL_L('a') <= c && c <= STRTOL_L('z') )
		return 10 + c - STRTOL_L('a');
	if ( STRTOL_L('A') <= c && c <= STRTOL_L('Z') )
		return 10 + c - STRTOL_L('A');
	return -1;
}

STRTOL_INT STRTOL(const STRTOL_CHAR* restrict str,
                  STRTOL_CHAR** restrict end_ptr,
                  int base)
{
	// Reject bad bases.
	if ( base < 0 || base == 1 || 36 < base )
	{
		if ( end_ptr )
			*end_ptr = (STRTOL_CHAR*) str;
		return errno = EINVAL, 0;
	}

	const STRTOL_CHAR* original_str = str;

	// Skip any leading white space.
	while ( STRTOL_ISSPACE((STRTOL_UCHAR) *str) )
		str++;

	bool negative = false;
	STRTOL_CHAR c = *str;

	// Handle a leading sign character.
	if ( c == STRTOL_L('-') )
		str++, negative = true;
	else if ( c == STRTOL_L('+') )
		str++, negative = false;

	bool actually_negative = !STRTOL_INT_IS_UNSIGNED && negative;

	// Autodetect base if requested.
	if ( base == 0 )
	{
		if ( str[0] == STRTOL_L('0') &&
		     (str[1] == STRTOL_L('x') || str[1] == STRTOL_L('X')) &&
		     (0 <= debase(str[2]) && debase(str[2]) < 16) )
			str += 2, base = 16;
		else if ( str[0] == STRTOL_L('0') &&
		          0 <= debase(str[1]) && debase(str[1]) < 8 )
			str++, base = 8;
		else
			base = 10;
	}

	// Skip the leading '0x' prefix in base 16 for hexadecimal integers.
	else if ( base == 16 )
	{
		if ( str[0] == STRTOL_L('0') &&
		     (str[1] == STRTOL_L('x') || str[1] == STRTOL_L('X')) &&
		     (0 <= debase(str[2]) && debase(str[2]) < 16) )
			str += 2;
	}

	// Determine what value will be returned on overflow/underflow.
	STRTOL_INT overflow_value =
		actually_negative ? STRTOL_INT_MIN : STRTOL_INT_MAX;

	// Convert a single character at a time.
	STRTOL_INT result = 0;
	size_t num_converted_chars = 0;
	bool overflow_occured = false;
	while ( (c = *str ) )
	{
		// Stop if we encounter a character that doesn't fit in this base.
		int val = debase(c);
		if ( val < 0 || base <= val )
			break;

		str++;
		num_converted_chars++;

		if ( overflow_occured )
			continue;

		// Attempt to multiply the accumulator with the current base.
		if ( __builtin_mul_overflow(result, base, &result) )
		{
			overflow_occured = true;
			result = overflow_value;
			errno = ERANGE;
			continue;
		}

		// Nothing needs to be added if we are encountered a zero digit.
		if ( val == 0 )
		{
		}

		// Attempt to add the latest digit to the accumulator (positive).
		else if ( !actually_negative &&
		          (STRTOL_INT) val <= (STRTOL_INT) (STRTOL_INT_MAX - result) )
		{
			result += (STRTOL_INT) val;
		}


		// Attempt to subtract the latest digit to the accumulator (negative).
		else if ( actually_negative &&
		          (STRTOL_UNSIGNED_INT) val <
		          ((STRTOL_UNSIGNED_INT) result -
		           (STRTOL_UNSIGNED_INT) STRTOL_INT_MIN) )
		{
			result -= (STRTOL_INT) val;
		}

		// Otherwise, the addition/subtract would overflow/underflow.
		else
		{
			overflow_occured = true;
			result = overflow_value;
			errno = ERANGE;
			continue;
		}
	}

	// If no characters were successfully converted, rewind to the start.
	if ( !num_converted_chars )
	{
		errno = EINVAL;
		str = original_str;
	}

	// Let the caller know where we got to.
	if ( end_ptr )
		*end_ptr = (STRTOL_CHAR*) str;

	// Handle the special case where we are creating an unsigned integer and the
	// string was negative. The result is the negation assuming no overflow.
	if ( STRTOL_INT_IS_UNSIGNED && negative )
	{
		if ( overflow_occured )
			result = STRTOL_INT_MAX;
		else
			result = -result;
	}

	return result;
}
