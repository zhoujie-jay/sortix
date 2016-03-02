/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdlib/mkostemps.c
 * Make a unique temporary file path and open it.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t NUM_CHARACTERS = 10 + 26 + 26;

static inline char random_character(void)
{
	uint32_t index = arc4random_uniform(NUM_CHARACTERS);
	if ( index < 10 )
		return '0' + index;
	if ( index < 10 + 26 )
		return 'a' + index - 10;
	if ( index < 10 + 26 + 26 )
		return 'A' + index - (10 + 26);
	__builtin_unreachable();
}

int mkostemps(char* templ, int suffixlen, int flags)
{
	size_t templ_length = strlen(templ);
	if ( templ_length < 6 ||
	     suffixlen < 0 ||
	     templ_length - 6 < (size_t) suffixlen )
		return errno = EINVAL, -1;
	size_t xpos = templ_length - (6 + suffixlen);
	for ( size_t i = 0; i < 6; i++ )
		if ( templ[xpos + i] != 'X' )
			return errno = EINVAL, -1;

	flags &= ~O_ACCMODE;
	do
	{
		for ( size_t i = 0; i < 6; i++ )
			templ[xpos + i] = random_character();
		int fd = open(templ, flags | O_RDWR | O_EXCL | O_CREAT, 0600);
		if ( 0 <= fd )
			return fd;
	} while ( errno == EEXIST );

	memcpy(templ + xpos, "XXXXXX", 6);

	return -1;
}
