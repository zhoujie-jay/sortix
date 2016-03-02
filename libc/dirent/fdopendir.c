/*
 * Copyright (c) 2011, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * dirent/fdopendir.c
 * Handles the file descriptor backend for the DIR* API on Sortix.
 */

#include <dirent.h>
#include <DIR.h>
#include <fcntl.h>
#include <stdlib.h>

DIR* fdopendir(int fd)
{
	int old_dflags = fcntl(fd, F_GETFD);
	if ( 0 <= old_dflags && !(old_dflags & FD_CLOEXEC) )
		fcntl(fd, F_SETFD, old_dflags | FD_CLOEXEC);
	// TODO: Potentially reopen as O_EXEC when the kernel requires that.
	DIR* dir = (DIR*) calloc(sizeof(DIR), 1);
	if ( !dir )
		return NULL;
	dir->fd = fd;
	return dir;
}
