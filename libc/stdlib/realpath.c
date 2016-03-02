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
 * stdlib/realpath.c
 * Return the canonicalized filename.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <scram.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef PATH_MAX
#error "This realpath implementation assumes no PATH_MAX"
#endif

char* realpath(const char* restrict path, char* restrict resolved_path)
{
	if ( resolved_path )
	{
		struct scram_undefined_behavior info;
		info.filename = __FILE__;
		info.line = __LINE__;
		info.column = 0;
		info.violation = "realpath call with non-null argument and PATH_MAX unset";
		scram(SCRAM_UNDEFINED_BEHAVIOR, &info);
	}
	char* ret_path = canonicalize_file_name(path);
	if ( !ret_path )
		return NULL;
	return ret_path;
}
