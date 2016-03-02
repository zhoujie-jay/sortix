/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/tmpfile.c
 * Opens an unnamed temporary file.
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE* tmpfile(void)
{
	// TODO: There is a short interval during which other processes can access
	//       this file. Implement and use O_TMPFILE.
	char path[] = "/tmp/tmp.XXXXXX";
	int fd = mkstemp(path);
	if ( fd < 0 )
		return (FILE*) NULL;
	if ( unlink(path) < 0 )
		return close(fd), (FILE*) NULL;
	FILE* fp = fdopen(fd, "r+");
	if ( !fp )
		return close(fd), (FILE*) NULL;
	return fp;
}
