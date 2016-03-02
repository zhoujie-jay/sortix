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
 * malloc/__heap_verify.c
 * Perform a heap consistency check.
 */

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

void __heap_verify(void)
{
	for ( size_t i = 0; i < sizeof(size_t) * 8 - 1; i++ )
	{
		if ( __heap_state.bin_filled_bitmap & heap_size_of_bin(i) )
		{
			assert(__heap_state.bin[i] != NULL);
		}
		else
		{
			assert(__heap_state.bin[i] == NULL);
		}
	}
}
