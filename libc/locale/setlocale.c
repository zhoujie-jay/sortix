/*
 * Copyright (c) 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * locale/setlocale.c
 * Set program locale.
 */

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

static char* current_locales[LC_NUM_CATEGORIES] = { NULL };

char* setlocale(int category, const char* locale)
{
	if ( category < 0 || LC_ALL < category )
		return errno = EINVAL, (char*) NULL;
	char* new_strings[LC_NUM_CATEGORIES];
	int from = category != LC_ALL ? category : 0;
	int to = category != LC_ALL ? category : LC_NUM_CATEGORIES - 1;
	if ( !locale )
		return current_locales[to] ? current_locales[to] : (char*) "C";
	for ( int i = from; i <= to; i++ )
	{
		if ( !(new_strings[i] = strdup(locale)) )
		{
			for ( int n = from; n < i; n++ )
				free(new_strings[n]);
			return (char*) NULL;
		}
	}
	for ( int i = from; i <= to; i++ )
	{
		free(current_locales[i]);
		current_locales[i] = new_strings[i];
	}
	return (char*) locale;
}
