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
 * netdb/gai_strerror.c
 * Error information for getaddrinfo.
 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

const char* gai_strerror(int err)
{
	switch ( err )
	{
	case EAI_AGAIN: return "Try again";
	case EAI_BADFLAGS: return "Invalid flags";
	case EAI_FAIL: return "Non-recoverable error";
	case EAI_FAMILY: return "Unrecognized address family or invalid length";
	case EAI_MEMORY: return "Out of memory";
	case EAI_NONAME: return "Name does not resolve";
	case EAI_SERVICE: return "Unrecognized service";
	case EAI_SOCKTYPE: return "Unrecognized socket type";
	case EAI_SYSTEM: return "System error";
	case EAI_OVERFLOW: return "Overflow";
	}
	return "Unknown error";
}
