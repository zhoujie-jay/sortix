/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
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
 * wctype/wctype.c
 * Returns the character class description with the given name.
 */

#include <string.h>
#include <wctype.h>

wctype_t wctype(const char *name)
{
	if ( !strcmp(name, "alnum") ) return (wctype_t) iswalnum;
	if ( !strcmp(name, "alpha") ) return (wctype_t) iswalpha;
	if ( !strcmp(name, "blank") ) return (wctype_t) iswblank;
	if ( !strcmp(name, "cntrl") ) return (wctype_t) iswcntrl;
	if ( !strcmp(name, "digit") ) return (wctype_t) iswdigit;
	if ( !strcmp(name, "graph") ) return (wctype_t) iswgraph;
	if ( !strcmp(name, "lower") ) return (wctype_t) iswlower;
	if ( !strcmp(name, "print") ) return (wctype_t) iswprint;
	if ( !strcmp(name, "punct") ) return (wctype_t) iswpunct;
	if ( !strcmp(name, "space") ) return (wctype_t) iswspace;
	if ( !strcmp(name, "upper") ) return (wctype_t) iswupper;
	if ( !strcmp(name, "xdigit") ) return (wctype_t) iswxdigit;
	return (wctype_t) NULL;
}
