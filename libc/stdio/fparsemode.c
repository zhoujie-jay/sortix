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
 * stdio/fparsemode.c
 * Parses the mode argument of functions like fopen().
 */

#include <errno.h>
#include <stdio.h>

int fparsemode(const char* mode)
{
	int result;

	switch ( *mode++ )
	{
	case 'r': result = FILE_MODE_READ; break;
	case 'w': result = FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE; break;
	case 'a': result = FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_APPEND; break;
	default: return errno = EINVAL, -1;
	};

	while ( *mode )
	{
		switch ( *mode++ )
		{
		case 'b': result |= FILE_MODE_BINARY; break;
		case 'e': result |= FILE_MODE_CLOEXEC; break;
		case 't': result &= ~FILE_MODE_BINARY; break;
		case 'x': result |= FILE_MODE_EXCL; break;
		case '+': result |= FILE_MODE_READ | FILE_MODE_WRITE; break;
		default: return errno = EINVAL, -1;
		};
	}

	return result;
}
