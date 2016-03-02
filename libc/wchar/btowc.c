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
 * wchar/btowc.c
 * Single byte to wide character conversion.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

wint_t btowc(int ic)
{
	if ( ic == EOF )
		return WEOF;
	unsigned char uc = (unsigned char) ic;
	char c = (char) uc;
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	wchar_t wc;
	int saved_errno = errno;
	size_t status = mbrtowc(&wc, (const char*) &c, 1, &ps);
	if ( status == (size_t) -1 || status == (size_t) -2 )
		return errno = saved_errno, WEOF;
	return (wint_t) wc;
}
