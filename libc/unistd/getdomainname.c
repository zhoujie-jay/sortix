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
 * unistd/getdomainname.c
 * Get the domainname.
 */

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int getdomainname(char* name, size_t len)
{
	const char* domainname = getenv("DOMAINNAME");
	if ( !domainname )
		domainname = "localdomain";
	size_t domainname_len = strlen(domainname);
	if ( len < domainname_len+1 )
		return errno = ENAMETOOLONG, -1;
	strcpy(name, domainname);
	return 0;
}