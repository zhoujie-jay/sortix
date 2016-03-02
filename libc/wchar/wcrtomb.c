/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * wchar/wcrtomb.c
 * Convert a wide character to a multibyte sequence.
 */

#include <errno.h>
#include <stdlib.h>
#include <wchar.h>

static
size_t utf8_wcrtomb(char* restrict s, wchar_t wc, mbstate_t* restrict ps)
{
	(void) ps;
	// The definition of UTF-8 prohibits encoding character numbers between
	// U+D800 and U+DFFF, which are reserved for use with the UTF-16 encoding
	// form (as surrogate pairs) and do not directly represent characters.
	if ( 0xD800 <= wc && wc <= 0xDFFF )
		return errno = EILSEQ, (size_t) -1;

	// RFC 3629 limits UTF-8 to 0x0 through 0x10FFFF.
	if ( 0x10FFFF <= wc )
		return errno = EILSEQ, (size_t) -1;

	size_t index = 0;

	if ( wc < (1 << (7)) )  /* 0xxxxxxx */
	{
		s[index++] = 0b00000000 | (wc >> 0 & 0b01111111);
		return index;
	}

	if ( wc < (1 << (5 + 1 * 6)) )  /* 110xxxxx 10xxxxxx^1 */
	{
		s[index++] = 0b11000000 | (wc >> 6 & 0b00011111);
		s[index++] = 0b10000000 | (wc >> 0 & 0b00111111);
		return index;
	}

	if ( wc < (1 << (4 + 2 * 6)) )  /* 1110xxxx 10xxxxxx^2 */
	{
		s[index++] = 0b11100000 | (wc >> 2*6 & 0b00001111);
		s[index++] = 0b10000000 | (wc >> 1*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 0*6 & 0b00111111);
		return index;
	}

	if ( wc < (1 << (3 + 3 * 6)) )  /* 11110xxx 10xxxxxx^3 */
	{
		s[index++] = 0b11110000 | (wc >> 3*6 & 0b00000111);
		s[index++] = 0b10000000 | (wc >> 2*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 1*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 0*6 & 0b00111111);
		return index;
	}

#if 0 /* 5-byte and 6-byte sequences are forbidden by RFC 3629 */
	if ( wc < (1 << (2 + 4 * 6)) )  /* 111110xx 10xxxxxx^4 */
	{
		s[index++] = 0b11111000 | (wc >> 4*6 & 0b00000011);
		s[index++] = 0b10000000 | (wc >> 3*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 2*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 1*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 0*6 & 0b00111111);
		return index;
	}

	if ( wc < (1 << (1 + 5 * 6)) )  /* 111110xx 10xxxxxx^5 */
	{
		s[index++] = 0b11111100 | (wc >> 5*6 & 0b00000001);
		s[index++] = 0b10000000 | (wc >> 4*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 3*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 2*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 1*6 & 0b00111111);
		s[index++] = 0b10000000 | (wc >> 0*6 & 0b00111111);
		return index;
	}
#endif

	return errno = EILSEQ; return (size_t) -1;
}

size_t wcrtomb(char* restrict s, wchar_t wc, mbstate_t* restrict ps)
{
	char internal_buffer[MB_CUR_MAX];
	if ( !s )
		wc = L'\0', s = internal_buffer;

	// TODO: Verify whether the current locale is UTF-8.
	return utf8_wcrtomb(s, wc, ps);
}
