/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013.

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

    stdlib/strtol.cpp
    Converts integers represented as strings to binary representation.

*******************************************************************************/

#ifndef STRTOL
#define STRTOL strtol
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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Nasty, nasty hack to get LLONG_* macros.
#define __STDC_VERSION__ 199901L
#include <limits.h>

// Convert a character into a digit.
static int debase(char c)
{
	if ( '0' <= c && c <= '9' )
		return c - '0';
	if ( 'a' <= c && c <= 'z' )
		return 10 + c - 'a';
	if ( 'A' <= c && c <= 'Z' )
		return 10 + c - 'A';
	return -1;
}

// Determine whether a multiplication of two integers would overflow/underflow.
// This is easy if we have a larger integer type, otherwise we'll be creative.
template <class T_INT,
          bool T_INT_IS_UNSIGNED,
          class T_UNSIGNED_INT,
          T_INT T_INT_MIN,
          T_INT T_INT_MAX>
static bool would_multiplication_overflow(T_INT a, T_INT b)
{
	// Prevent accidental divisons by zero in this simple case.
	if ( !a || !b )
		return false;

	// Check if we have a 64-bit integer that it is large enough.
	if ( sizeof(T_INT)*2 <= sizeof(int64_t) )
	{
		int64_t ret_large = (int64_t) a * (int64_t) b;
		return ret_large < (int64_t) T_INT_MIN ||
		       (int64_t) T_INT_MAX < ret_large;
	}

	// Check if we have a 128-bit integer that it is large enough.
#if 64 <= __WORDSIZE
	if ( sizeof(T_INT)*2 <= sizeof(__int128) )
	{
		__int128 ret_large = (__int128) a * (__int128) b;
		return ret_large < (__int128) T_INT_MIN ||
		       (__int128) T_INT_MAX < ret_large;
	}
#endif

	// The fallback strategy is to determine the largest b given a that will not
	// overflow and then see if b is within range. This is trivial if in the
	// unsigned integer case.
	if ( T_INT_IS_UNSIGNED )
	{
		T_INT max_b = a / T_INT_MAX;
		return max_b < b;
	}

	// We have to deal with some cases for signed integers. We'll assume signed
	// integers are in two's complement and use - (unsigned int) value to take
	// the absolute value of a negative value as an unsigned integer in a manner
	// that is defined in C. Whether we use the smallest or largest value depend
	// on whether the sign of a and b is identical.
	else
	{
		T_UNSIGNED_INT a_abs = a < 0 ? - (T_UNSIGNED_INT) a : a;
		T_UNSIGNED_INT b_abs = b < 0 ? - (T_UNSIGNED_INT) b : b;
		T_UNSIGNED_INT min_abs = - (T_UNSIGNED_INT) T_INT_MIN;
		T_UNSIGNED_INT max_abs = T_INT_MAX;
		T_UNSIGNED_INT limit_pos = (0 <= a && 0 <= b) || ( a < 0 && b < 0 );
		T_UNSIGNED_INT limit = limit_pos ? max_abs : min_abs;
		T_UNSIGNED_INT max_b = a_abs / limit;
		return max_b < b_abs;
	}
}

extern "C"
STRTOL_INT STRTOL(const char* restrict str, char** restrict endptr, int base)
{
	const char* origstr = str;
	int origbase = base;

	// Skip any leading white space.
	while ( isspace(*str) )
		str++;

	// Reject bad bases.
	if ( base < 0 || 36 < base )
	{
		if ( endptr )
			*endptr = (char*) str;
		return errno = EINVAL, 0;
	}

	bool negative = false;
	char c = *str;

	// Handle a leading sign character.
	if ( c == '-' )
		str++, negative = true;
	if ( c == '+' )
		str++, negative = false;

	// Autodetect base 8 or base 16.
	if ( !base && str[0] == '0' )
	{
		if ( str[1] == 'x' || str[1] == 'X' )
			str += 2, base = 16;
		else if ( 0 <= debase(str[1]) && debase(str[1]) < 8 )
			str++, base = 8;
	}

	// Default to base 10.
	if ( !base )
		base = 10;

	// Skip the leading '0x' prefix in base 16 for hexadecimal integers.
	if ( origbase == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
		str += 2;

	// Determine what value will be returned on overflow/underflow.
	STRTOL_INT overflow_value = negative && !STRTOL_INT_IS_UNSIGNED ?
	                            STRTOL_INT_MIN :
	                            STRTOL_INT_MAX;

	// Convert a single character at a time.
	STRTOL_INT result = 0;
	size_t numconvertedchars = 0;
	bool overflow_occured = false;
	while ( (c = *str ) )
	{
		// Stop if we encounterd a character that doesn't fit in this base.
		int val = debase(c);
		if ( val < 0 || base <= val )
			break;

		// Attempt to multiply the accumulator with the current base.
		if ( would_multiplication_overflow<STRTOL_INT,
		                                   STRTOL_INT_IS_UNSIGNED,
		                                   STRTOL_UNSIGNED_INT,
		                                   STRTOL_INT_MIN,
		                                   STRTOL_INT_MAX>
		                                  (result, (STRTOL_INT) base) )
			overflow_occured = true, result = overflow_value;
		else
		{
			STRTOL_INT new_result = result * (STRTOL_INT) base;
			assert(result <= new_result);
			result = new_result;
		}

		// Attempt to add the latest digit to the accumulator (positive).
		if ( (STRTOL_INT_IS_UNSIGNED || !negative) &&
		     (STRTOL_INT) val <= (STRTOL_INT) (STRTOL_INT_MAX - result) )
			result += (STRTOL_INT) val;

		// Attempt to subtract the latest digit to the accumulator (negative).
		else if ( (!STRTOL_INT_IS_UNSIGNED && negative) &&
		          (STRTOL_INT) val <= (STRTOL_INT) (result - STRTOL_INT_MIN) )
			result -= (STRTOL_INT) val;

		// Handle the case where the addition/subtract would overflow/underflow.
		else
			overflow_occured = true, result = overflow_value;

		str++;
		numconvertedchars++;
	}

	// If no characters were successfully converted, rewind to the start, also
	// rewinding past skipped whitespace and sign characters and such.
	if ( !numconvertedchars )
		str = origstr, result = 0;

	// Let the caller know where we got to.
	if ( endptr )
		*endptr = (char*) str;

	// Handle the special case where we are creating an unsigned integer and the
	// string was negative and non-zero and no overflow occured, then we treat
	// it as (the maximum value+1) minus (the negative string as integer).
	if ( STRTOL_INT_IS_UNSIGNED && negative && result && !overflow_occured )
		result = STRTOL_INT_MAX - (result-1);

	return result;
}