/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * wchar/mbsnrtowcs.c
 * Convert a multibyte string to a wide-character string.
 */

#include <assert.h>
#include <wchar.h>

size_t mbsnrtowcs(wchar_t* restrict dst,
                  const char** restrict src_ptr,
                  size_t src_len,
                  size_t dst_len,
                  mbstate_t* restrict ps)
{
	static mbstate_t static_ps;
	if ( !ps )
		ps = &static_ps;

	assert(src_ptr && *src_ptr);
	const char* src = *src_ptr;

	// Continue to decode wide characters until we have filled the destination
	// buffer or if we have exhausted the limit on input multibyte characters.
	size_t dst_offset = 0;
	size_t src_offset = 0;
	while ( (!dst || dst_offset < dst_len) && src_offset < src_len )
	{
		mbstate_t ps_copy = *ps;
		wchar_t wc;
		size_t amount = mbrtowc(&wc, src + src_offset, src_len - src_offset, ps);

		// Stop in the event a decoding error occured.
		if ( amount == (size_t) -1 )
			return *src_ptr = src + src_offset, (size_t) -1;

		// Stop decoding early in the event we encountered a partial character.
		if ( amount == (size_t) -2 )
		{
			*ps = ps_copy;
			break;
		}

		// Store the decoded wide character in the destination buffer.
		if ( dst )
			dst[dst_offset] = wc;

		// Stop decoding after decoding a null character and return a NULL
		// source pointer to the caller, not including the null character in the
		// number of characters stored in the destination buffer.
		if ( wc == L'\0' )
		{
			src = NULL;
			src_offset = 0;
			break;
		}

		dst_offset++;
		src_offset += amount;
	}

	return *src_ptr = src + src_offset, dst_offset;
}
