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
 * malloc/heap_get_paranoia.c
 * Returns how paranoid the heap implementation should be.
 */

#include <malloc.h>
#include <stdlib.h>

int heap_get_paranoia(void)
{
#if defined(PARANOIA_DEFAULT) && defined(__is_sortix_libk)
	return PARANOIA_DEFAULT;
#elif defined(PARANOIA_DEFAULT)
	static int cached_paranoia = -1;
	if ( cached_paranoia < 0 )
	{
		if ( const char* paranoia_str = getenv("LIBC_MALLOC_PARANOIA") )
			cached_paranoia = atoi(paranoia_str);
		else
			cached_paranoia = PARANOIA_DEFAULT;
	}
	return cached_paranoia;
#else
	return PARANOIA;
#endif
}
