/*	$OpenBSD: timingsafe_memcmp.c,v 1.1 2014/06/13 02:12:17 matthew Exp $	*/
/*
 * Copyright (c) 2014 Google Inc.
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
 */

#include <limits.h>
#include <string.h>

// TODO: I don't fully trust that this code is _always_ timing safe in the
//       presence of very smart compilers that can prove that this computes the
//       same value as memcmp and could be short-circuited. We should invent and
//       rely on compiler extensions that inform the compiler that sensitive
//       information is involved, which forbids the compiler code generation
//       from branching on the information (and such).
// TODO: We should add testcases that verify this is actually timing safe.

int timingsafe_memcmp(const void* a_ptr, const void* b_ptr, size_t size)
{
	const unsigned char* a = (const unsigned char*) a_ptr;
	const unsigned char* b = (const unsigned char*) b_ptr;
	int result = 0;
	int done = 0;
	for ( size_t i = 0; i < size; i++ )
	{
		/* lt is -1 if a[i] < b[i]; else 0. */
		int lt = (a[i] - b[i]) >> CHAR_BIT;

		/* gt is -1 if a[i] > b[i]; else 0. */
		int gt = (b[i] - a[i]) >> CHAR_BIT;

		/* cmp is 1 if a[i] > b[i]; -1 if a[i] < b[i]; else 0. */
		int cmp = lt - gt;

		/* set result = cmp if !done. */
		result |= cmp & ~done;

		/* set done if a[i] != b[i]. */
		done |= lt | gt;
	}
	return result;
}
