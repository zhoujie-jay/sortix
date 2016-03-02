/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
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
 * unistd/get_current_dir_name.c
 * Returns the current working directory.
 */

#include <sys/stat.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: This interface should removed as its $PWD use is not thread safe.
char* get_current_dir_name(void)
{
	const char* pwd = getenv("PWD");
	int saved_errno = errno;
	struct stat pwd_st;
	struct stat cur_st;
	if ( pwd && pwd[0] && stat(pwd, &pwd_st) == 0 && stat(".", &cur_st) == 0 )
	{
		if ( cur_st.st_dev == pwd_st.st_dev && cur_st.st_ino == pwd_st.st_ino )
			return strdup(pwd);
	}
	errno = saved_errno;
	return canonicalize_file_name(".");
}
