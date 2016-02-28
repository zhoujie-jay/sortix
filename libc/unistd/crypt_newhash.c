/* $OpenBSD: cryptutil.c,v 1.12 2015/09/13 15:33:48 guenther Exp $ */
/*
 * Copyright (c) 2014 Ted Unangst <tedu@openbsd.org>
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
 */
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
crypt_checkpass(const char *pass, const char *goodhash)
{
	char dummy[128];

	if (goodhash == NULL) {
		/* fake it */
		goto fake;
	}

	/* empty password */
	if (strlen(goodhash) == 0 && strlen(pass) == 0)
		return 0;

	if (goodhash[0] == '$' && goodhash[1] == '2') {
		if (bcrypt_checkpass(pass, goodhash))
			goto fail;
		return 0;
	}

	/* unsupported. fake it. */
fake:
	bcrypt_newhash(pass, 8, dummy, sizeof(dummy));
fail:
	errno = EACCES;
	return -1;
}

int
crypt_newhash(const char *pass, const char *pref, char *hash, size_t hashlen)
{
	int rv = -1;
	const char *defaultpref = "blowfish,8";
	int rounds;

	if (pref == NULL)
		pref = defaultpref;
	if (strncmp(pref, "blowfish", 8) == 0)
		pref += 8;
	else if (strncmp(pref, "bcrypt", 6) == 0)
		pref += 6;
	else {
		errno = EINVAL;
		goto err;
	}
	if (!*pref)
		rounds = -1; /* auto */
	else if (*pref++ != ',') {
		errno = EINVAL;
		goto err;
	} else if (strcmp(pref, "a") == 0)
		rounds = -1; /* auto */
	else {
		const char* pref_end;
		long rounds_long = strtol((char*) pref, (char**) &pref_end, 10);
		if (*pref_end || rounds_long < 4 || 31 < rounds_long) {
			errno = EINVAL;
			goto err;
		}
		rounds = rounds_long;
	}
	rv = bcrypt_newhash(pass, rounds, hash, hashlen);

err:
	return rv;
}
