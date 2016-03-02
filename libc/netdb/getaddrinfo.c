/*
 * Copyright (c) 2013, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * netdb/getaddrinfo.c
 * Network address and service translation.
 */

#include <sys/socket.h>

#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool linkaddrinfo(struct addrinfo** restrict* res_ptr,
                         const struct addrinfo* restrict templ)
{
	struct addrinfo* link = (struct addrinfo*) calloc(1, sizeof(struct addrinfo));
	if ( !link )
		return false;
	link->ai_flags = templ->ai_flags;
	link->ai_family = templ->ai_family;
	link->ai_socktype = templ->ai_socktype;
	link->ai_protocol = templ->ai_protocol;
	link->ai_addrlen = templ->ai_addrlen;
	link->ai_addr = (struct sockaddr*) malloc(templ->ai_addrlen);
	if ( !link->ai_addr )
		return free(link), false;
	memcpy(link->ai_addr, templ->ai_addr, templ->ai_addrlen);
	link->ai_canonname = templ->ai_canonname ? strdup(templ->ai_canonname) : NULL;
	if ( templ->ai_canonname && !link->ai_canonname )
		return free(link), free(link->ai_addr), false;
	**res_ptr = link;
	*res_ptr = &link->ai_next;
	return true;
}

int getaddrinfo(const char* restrict node,
                const char* restrict servname,
                const struct addrinfo* restrict hints,
                struct addrinfo** restrict res)
{
	struct addrinfo hints_def;
	if ( !hints )
	{
		memset(&hints_def, 0, sizeof(hints_def));
		hints_def.ai_family = AF_UNSPEC;
		hints = &hints_def;
	}

	int socktype = hints->ai_socktype;
	if ( socktype == 0 )
		socktype = SOCK_STREAM;

	in_port_t port = 0;
	if ( servname )
	{
		if ( isspace((unsigned char) servname[0]) )
			return EAI_SERVICE;
		const char* end;
		long portl = strtol(servname, (char**) &end, 10);
		if ( end[0] )
			return EAI_SERVICE;
		if ( (in_port_t) portl != portl )
			return EAI_SERVICE;
		port = portl;
	}

	struct addrinfo** res_orig = res;
	*res = NULL;

	if ( !node )
	{
		bool any = false;
		if ( hints->ai_family == AF_UNSPEC || hints->ai_family == AF_INET )
		{
			any = true;
			struct sockaddr_in sin;
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_port = htons(port);
			memcpy(&sin.sin_addr, "\x7F\x00\x00\x01", 4);
			struct addrinfo templ;
			memset(&templ, 0, sizeof(templ));
			templ.ai_family = sin.sin_family;
			templ.ai_socktype = socktype;
			templ.ai_protocol = hints->ai_protocol;
			templ.ai_addrlen = sizeof(sin);
			templ.ai_addr = (struct sockaddr*) &sin;
			if ( !linkaddrinfo(&res, &templ) )
				return freeaddrinfo(*res_orig), EAI_MEMORY;
		}
		if ( any )
			return 0;
	}

	return EAI_NONAME;
}
