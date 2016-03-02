/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * fsfunc.cpp
 * Filesystem related utility functions.
 */

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/dirent.h>
#include <sortix/stat.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/string.h>

#include <string.h>

namespace Sortix {

unsigned char ModeToDT(mode_t mode)
{
	if ( S_ISSOCK(mode) )
		return DT_SOCK;
	if ( S_ISLNK(mode) )
		return DT_LNK;
	if ( S_ISREG(mode) )
		return DT_REG;
	if ( S_ISBLK(mode) )
		return DT_BLK;
	if ( S_ISDIR(mode) )
		return DT_DIR;
	if ( S_ISCHR(mode) )
		return DT_CHR;
	if ( S_ISFIFO(mode) )
		return DT_FIFO;
	return DT_UNKNOWN;
}

// '' -> '' ''
// '/' -> '' '/'
// '///' -> '' '///'
// '.' -> '' '.'
// 'test' -> '' 'test'
// 'test/dir' -> 'test/' 'dir'
// 'test/dir/foo' -> 'test/dir/' 'foo'
// 'test/dir/' -> 'test/' 'dir/'
// '../' -> '' '../'
// 'foo///bar//test///' -> 'foo///bar//' 'test///'

bool SplitFinalElem(const char* path, char** dir, char** final)
{
	size_t pathlen = strlen(path);
	size_t splitat = pathlen;
	while ( splitat && path[splitat-1] == '/' )
		splitat--;
	while ( splitat && path[splitat-1] != '/' )
		splitat--;
	char* retdir = String::Substring(path, 0, splitat);
	if ( !retdir ) { return false; }
	char* retfinal = String::Substring(path, splitat, pathlen - splitat);
	if ( !retfinal ) { delete[] retdir; return false; }
	*dir = retdir;
	*final = retfinal;
	return true;
}

} // namespace Sortix
