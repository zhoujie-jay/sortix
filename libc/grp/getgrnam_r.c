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
 * grp/getgrnam_r.c
 * Searchs the group database for a group with the given groupname.
 */

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>

int getgrnam_r(const char* restrict groupname,
               struct group* restrict ret,
               char* restrict buf,
               size_t buflen,
               struct group** restrict ret_ptr)
{
	if ( !ret_ptr )
		return errno = EINVAL;
	if ( !groupname || !ret || !buf )
		return *ret_ptr = NULL, errno = EINVAL;
	FILE* fgroup = opengr();
	if ( !fgroup )
		return *ret_ptr = NULL, errno;
	int errnum;
	while ( (errnum = fgetgrent_r(fgroup, ret, buf, buflen, ret_ptr)) == 0 &&
	        *ret_ptr )
	{
		if ( strcmp((*ret_ptr)->gr_name, groupname) != 0 )
			continue;
		fclose(fgroup);
		return *ret_ptr = *ret_ptr, 0;
	}
	fclose(fgroup);
	return *ret_ptr = NULL, errnum ? errnum : (errno = ENOGROUP);
}
