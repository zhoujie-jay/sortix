/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * string/strverscmp.c
 * Compares two version strings.
 */

#include <stdbool.h>
#include <string.h>

static bool is_number(char c)
{
	return '0' <= c && c <= '9';
}

static int to_number(char c)
{
	return c - '0';
}

int strverscmp(const char* a, const char* b)
{
	for ( size_t i = 0; true; i++ )
	{
		// Be a regular strcmp if the strings are equal.
		if ( a[i] == '\0' && b[i] == '\0' )
			return 0;
		if ( a[i] == b[i] )
			continue;

		// Be a regular strcmp if no digits are involed when they differ.
		bool version_string = is_number(a[i]) && is_number(b[i]);
		if ( !version_string && a[i] < b[i] )
			return -1;
		if ( !version_string && a[i] > b[i] )
			return 1;

		// Because the number of leading zeroes matter, we have to find the
		// entire numeric block we are currently within. We know the strings are
		// equal until i, so we can simply find the number of shared digits by
		// looking in the first string.
		size_t digits_start = i;
		while ( digits_start && is_number(a[digits_start-1]) )
			digits_start--;
		size_t shared_digits = i - digits_start;

		// Find the number of shared leading zeroes.
		size_t shared_zeroes = 0;
		while ( shared_zeroes < shared_digits &&
		        to_number(a[digits_start + shared_zeroes]) == 0 )
			shared_zeroes++;

		// Try to expand the leading zeroes amount into a.
		size_t a_zeroes = shared_zeroes;
		while ( is_number(a[digits_start + a_zeroes]) == 0 )
			a_zeroes++;

		// Try to expand the leading zeroes amount into b.
		size_t b_zeroes = shared_zeroes;
		while ( is_number(b[digits_start + b_zeroes]) == 0 )
			b_zeroes++;

		// We treat strings with leading zeroes as if they have a decimal point
		// in front of them, so strings with more zeroes sort lower.
		if ( a_zeroes > b_zeroes )
			return -1;
		if ( b_zeroes > a_zeroes )
			return 1;

		// Find the number of consecutive digits in a where the strings differ.
		size_t a_digits = a_zeroes;
		while ( is_number(a[digits_start + a_digits]) )
			a_digits++;

		// Find the number of consecutive digits in b where the strings differ.
		size_t b_digits = b_zeroes;
		while ( is_number(b[digits_start + b_digits]) )
			b_digits++;

		// We know the strings have the same amount of leading zeroes, so we
		// so if a a block is longer than the other, then the value must be
		// longer as well.
		if ( a_digits < b_digits )
			return -1;
		if ( b_digits < a_digits )
			return 1;

		// Finally run through the strings from where they differ and sort them
		// numerically. We know this terminates because the strings differ. The
		// strings have the same amount of digits, so comparing them is easy.
		for ( size_t n = shared_zeroes; true; n++ )
		{
			if ( a[digits_start + n] < b[digits_start + n] )
				return -1;
			if ( b[digits_start + n] < a[digits_start + n] )
				return 1;
		}
	}
}
