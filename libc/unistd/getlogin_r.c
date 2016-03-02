/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * unistd/getlogin_r.c
 * Get name of user logged in at the controlling terminal.
 */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int getlogin_r(char* buf, size_t size)
{
	struct passwd passwd_object;
	struct passwd* passwd;
	int errnum = 0;

	char* pwdbuf = NULL;
	size_t pwdbuflen = 0;
	do
	{
		// TODO: Potential overflow.
		size_t new_pwdbuflen = pwdbuflen ? 2 * pwdbuflen : 64;
		char* new_pwdbuf = (char*) realloc(pwdbuf, new_pwdbuflen);
		if ( !new_pwdbuf )
			return free(pwdbuf), -1;
		pwdbuf = new_pwdbuf;
		pwdbuflen = new_pwdbuflen;
	} while ( (errnum = getpwuid_r(getuid(), &passwd_object, pwdbuf, pwdbuflen,
	                               &passwd)) == ERANGE );
	if ( errnum )
		return free(pwdbuf), errno = errnum, -1;

	const char* username = passwd->pw_name;
	if ( size <= strlcpy(buf, username, size) )
		return free(pwdbuf), errno = ERANGE, -1;
	free(pwdbuf);

	return 0;
}
