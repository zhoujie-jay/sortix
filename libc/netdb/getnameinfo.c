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
 * netdb/getnameinfo.c
 * Address-to-name translation in protocol-independent manner.
 */

#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>

int getnameinfo(const struct sockaddr* restrict sa,
                socklen_t salen,
                char* restrict host,
                socklen_t hostlen,
                char* restrict serv,
                socklen_t servlen,
                int flags)
{
	(void) sa;
	(void) salen;
	(void) host;
	(void) hostlen;
	(void) serv;
	(void) servlen;
	(void) flags;
	fprintf(stderr, "%s is not implemented, aborting.\n", __func__);
	abort();
}
