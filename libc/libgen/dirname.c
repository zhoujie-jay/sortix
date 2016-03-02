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
 * libgen/dirname.c
 * Returns the directory part of a path.
 */

#include <libgen.h>
#include <string.h>

static const char current_directory[2] = ".";

char* dirname(char* path)
{
	if ( !path || !*path )
		return (char*) current_directory;
	size_t path_len = strlen(path);
	while ( 2 <= path_len && path[path_len-1] == '/' )
		path[--path_len] = '\0';
	if ( strcmp(path, "/") == 0 )
		return path;
	if ( !strchr(path, '/') )
		return (char*) current_directory;
	while ( 1 <= path_len && path[path_len-1] != '/' )
		path[--path_len] = '\0';
	while ( 1 <= path_len && path[path_len-1] == '/' )
		path[--path_len] = '\0';
	if ( !path[0] )
		path[0] = '/', path[1] = '\0';
	return path;
}
